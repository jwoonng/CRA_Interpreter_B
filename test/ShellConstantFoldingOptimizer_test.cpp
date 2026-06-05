#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include "src/executor/IExecutor.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"

// ── Test Double: SpyExecutor ──────────────────────────────────────────────
// execute() 호출 시 Shell 파이프라인이 넘겨준 AST의 BinaryExpr 노드 개수를
// 정적으로 세고, 실제 실행은 내부 Executor에 위임한다.
// BinaryExpr 노드가 0개라면 런타임에 해당 연산이 단 한 번도 평가되지 않음을 의미한다.
// countOnly = true 로 설정하면 AST 집계만 수행하고 실제 실행을 생략한다.
// (루프 반복 횟수가 매우 클 때 테스트 속도 보호 목적)
struct SpyExecutor : IExecutor {
    int  binaryExprCount = 0;
    bool countOnly       = false;

    void execute(const std::vector<std::unique_ptr<Stmt>>& stmts,
                 std::ostream& out) override {
        binaryExprCount = 0;
        for (const auto& s : stmts)
            binaryExprCount += countStmt(s.get());
        if (!countOnly)
            real_.execute(stmts, out);
    }

private:
    Executor real_;

    int countExpr(Expr* e) {
        if (!e) return 0;
        if (auto* b = dynamic_cast<BinaryExpr*>(e))
            return 1 + countExpr(b->left.get()) + countExpr(b->right.get());
        if (auto* u = dynamic_cast<UnaryExpr*>(e))    return countExpr(u->right.get());
        if (auto* g = dynamic_cast<GroupingExpr*>(e)) return countExpr(g->expression.get());
        if (auto* a = dynamic_cast<AssignExpr*>(e))   return countExpr(a->value.get());
        if (auto* l = dynamic_cast<LogicalExpr*>(e))
            return countExpr(l->left.get()) + countExpr(l->right.get());
        return 0; // LiteralExpr, VariableExpr
    }

    int countStmt(Stmt* s) {
        if (!s) return 0;
        if (auto* es = dynamic_cast<ExpressionStmt*>(s)) return countExpr(es->expression.get());
        if (auto* ps = dynamic_cast<PrintStmt*>(s))      return countExpr(ps->expression.get());
        if (auto* vs = dynamic_cast<VarDeclareStmt*>(s)) return countExpr(vs->initializer.get());
        if (auto* bs = dynamic_cast<BlockStmt*>(s)) {
            int total = 0;
            for (const auto& inner : bs->statements) total += countStmt(inner.get());
            return total;
        }
        if (auto* is = dynamic_cast<IfStmt*>(s))
            return countExpr(is->condition.get())
                 + countStmt(is->thenBranch.get())
                 + countStmt(is->elseBranch.get());
        if (auto* fs = dynamic_cast<ForStmt*>(s))
            return countStmt(fs->initializer.get())
                 + countExpr(fs->condition.get())
                 + countExpr(fs->increment.get())
                 + countStmt(fs->body.get());
        return 0;
    }
};

// ── Test Double: RuntimeSpyBinaryExpr / RuntimeSpyOptimizer ──────────────
// RuntimeSpyBinaryExpr: accept() 호출마다 공유 카운터를 증가시켜
//   루프 반복을 포함한 실제 런타임 평가 횟수를 추적한다.
// RuntimeSpyOptimizer : AST의 모든 BinaryExpr를 RuntimeSpyBinaryExpr로 교체한다.
//   ConstantFoldingOptimizer 이후에 체인에 추가하면, 접히고 남은 BinaryExpr만
//   카운트되어 최적화 전후 런타임 평가 횟수를 직접 비교할 수 있다.
namespace {

struct RuntimeSpyBinaryExpr : BinaryExpr {
    std::shared_ptr<int> callCount;
    RuntimeSpyBinaryExpr(ExprPtr left, Token op, ExprPtr right,
                         std::shared_ptr<int> count)
        : BinaryExpr(std::move(left), std::move(op), std::move(right))
        , callCount(std::move(count)) {}
    LiteralValue accept(ExprVisitor& v) override {
        ++(*callCount);
        return v.visitBinaryExpr(*this);
    }
};

struct RuntimeSpyOptimizer : IOptimizer {
    std::shared_ptr<int> callCount;
    explicit RuntimeSpyOptimizer(std::shared_ptr<int> count)
        : callCount(std::move(count)) {}

    std::vector<StmtPtr> optimize(std::vector<StmtPtr> stmts) override {
        for (auto& s : stmts) wrapStmt(s);
        return std::move(stmts);
    }

private:
    ExprPtr wrapExpr(ExprPtr expr) {
        if (!expr) return expr;
        if (auto* b = dynamic_cast<BinaryExpr*>(expr.get())) {
            auto left  = wrapExpr(std::move(b->left));
            auto right = wrapExpr(std::move(b->right));
            auto op    = b->op;
            return std::make_unique<RuntimeSpyBinaryExpr>(
                std::move(left), op, std::move(right), callCount);
        }
        if (auto* g = dynamic_cast<GroupingExpr*>(expr.get()))
            g->expression = wrapExpr(std::move(g->expression));
        else if (auto* u = dynamic_cast<UnaryExpr*>(expr.get()))
            u->right = wrapExpr(std::move(u->right));
        else if (auto* a = dynamic_cast<AssignExpr*>(expr.get()))
            a->value = wrapExpr(std::move(a->value));
        else if (auto* l = dynamic_cast<LogicalExpr*>(expr.get())) {
            l->left  = wrapExpr(std::move(l->left));
            l->right = wrapExpr(std::move(l->right));
        }
        return expr;
    }

    void wrapStmt(StmtPtr& stmt) {
        if (!stmt) return;
        if (auto* s = dynamic_cast<ExpressionStmt*>(stmt.get()))
            s->expression = wrapExpr(std::move(s->expression));
        else if (auto* s = dynamic_cast<PrintStmt*>(stmt.get()))
            s->expression = wrapExpr(std::move(s->expression));
        else if (auto* s = dynamic_cast<VarDeclareStmt*>(stmt.get()))
            s->initializer = wrapExpr(std::move(s->initializer));
        else if (auto* s = dynamic_cast<BlockStmt*>(stmt.get()))
            for (auto& inner : s->statements) wrapStmt(inner);
        else if (auto* s = dynamic_cast<IfStmt*>(stmt.get())) {
            s->condition = wrapExpr(std::move(s->condition));
            wrapStmt(s->thenBranch);
            wrapStmt(s->elseBranch);
        } else if (auto* s = dynamic_cast<ForStmt*>(stmt.get())) {
            wrapStmt(s->initializer);
            s->condition = wrapExpr(std::move(s->condition));
            s->increment = wrapExpr(std::move(s->increment));
            wrapStmt(s->body);
        }
    }
};

} // namespace

// ── 헬퍼 ──────────────────────────────────────────────────────────────────
// 동일 소스를 최적화 유무에 따라 실행하고, Executor가 받은 BinaryExpr 개수를 반환한다.
// countOnly = true 이면 AST 집계만 수행하고 실제 코드는 실행하지 않는다.
static int binaryExprCountFor(const std::string& source, bool withOptimizer,
                               bool countOnly = false) {
    auto spy = std::make_unique<SpyExecutor>();
    spy->countOnly = countOnly;
    SpyExecutor* spyPtr = spy.get();

    Shell shell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::move(spy)
    );
    if (withOptimizer)
        shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());

    shell.runLine(source);
    return spyPtr->binaryExprCount;
}

// ConstantFoldingOptimizer(선택) → RuntimeSpyOptimizer 순으로 체인에 추가하고
// 실제 실행하여 BinaryExpr.accept() 호출 횟수(런타임)를 반환한다.
static int runtimeBinaryExprCountFor(const std::string& source,
                                     bool withConstantFolding) {
    auto count = std::make_shared<int>(0);

    Shell shell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::make_unique<Executor>()
    );
    if (withConstantFolding)
        shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    shell.addOptimizer(std::make_unique<RuntimeSpyOptimizer>(count));

    shell.runLine(source);
    return *count;
}

// ════════════════════════════════════════════════════
// Wave 1 — 순수 상수 수식: BinaryExpr 0개로 감소
// ════════════════════════════════════════════════════

// print 2 + 3;
// 최적화 전: BinaryExpr(+, 2, 3)  → 1개
// 최적화 후: LiteralExpr(5)        → 0개
TEST(ShellConstantFoldingOptimizerTest, SingleConstant_ReducedToZero) {
    EXPECT_EQ(binaryExprCountFor("print 2 + 3;", false), 1);
    EXPECT_EQ(binaryExprCountFor("print 2 + 3;", true),  0);
}

// print (2 + 3) * (4 + 5);
// 최적화 전: BinaryExpr 3개 (*, 첫번째 +, 두번째 +)
// 최적화 후: LiteralExpr(45)  → 0개
TEST(ShellConstantFoldingOptimizerTest, NestedConstant_AllReducedToZero) {
    EXPECT_EQ(binaryExprCountFor("print (2 + 3) * (4 + 5);", false), 3);
    EXPECT_EQ(binaryExprCountFor("print (2 + 3) * (4 + 5);", true),  0);
}

// var a = 5 * 4;
// 최적화 전: VarDeclare initializer에 BinaryExpr 1개
// 최적화 후: LiteralExpr(20) → 0개
TEST(ShellConstantFoldingOptimizerTest, ConstantInitializer_ReducedToZero) {
    EXPECT_EQ(binaryExprCountFor("var a = 5 * 4;", false), 1);
    EXPECT_EQ(binaryExprCountFor("var a = 5 * 4;", true),  0);
}

// ════════════════════════════════════════════════════
// Wave 2 — 루프: 본문 상수만 제거, 제어식은 유지
// ════════════════════════════════════════════════════

// for(var i=0; i<3; i=i+1) { print 2+3; }
// 최적화 전: i<3(1) + i+1(1) + 2+3(1) = 3개
// 최적화 후: i<3(1) + i+1(1) + LiteralExpr(5)(0) = 2개
//   → 루프 본문의 상수 BinaryExpr만 제거되고 제어식은 변수를 포함하므로 유지됨
TEST(ShellConstantFoldingOptimizerTest, LoopBody_ConstantExprRemoved_ControlFlowRetained) {
    EXPECT_EQ(binaryExprCountFor("for(var i=0; i<3; i=i+1) { print 2+3; }", false), 3);
    EXPECT_EQ(binaryExprCountFor("for(var i=0; i<3; i=i+1) { print 2+3; }", true),  2);
}

// ════════════════════════════════════════════════════
// Wave 3 — 부분 접기: 상수 서브트리만 제거
// ════════════════════════════════════════════════════

// var x = 10; print x + 1 * 2;
// 파싱: BinaryExpr(+, VariableExpr(x), BinaryExpr(*, 1, 2))
// 최적화 전: 2개 (+ 와 *)
// 최적화 후: 1개 (1*2는 LiteralExpr(2)로 접힘, x+2는 변수 포함으로 유지)
TEST(ShellConstantFoldingOptimizerTest, MixedExpr_ConstantSubtreeRemoved) {
    const std::string src = "var x = 10; print x + 1 * 2;";
    EXPECT_EQ(binaryExprCountFor(src, false), 2);
    EXPECT_EQ(binaryExprCountFor(src, true),  1);
}

// ════════════════════════════════════════════════════
// Wave 4 — 순수 변수 수식: 접기 없음 (횟수 동일)
// ════════════════════════════════════════════════════

// var x=5; var y=3; print x + y;
// 최적화 전후 모두 BinaryExpr 1개 (변수끼리의 연산은 접을 수 없음)
TEST(ShellConstantFoldingOptimizerTest, VariableExpr_CountUnchanged) {
    const std::string src = "var x = 5; var y = 3; print x + y;";
    int without = binaryExprCountFor(src, false);
    int with    = binaryExprCountFor(src, true);
    EXPECT_EQ(without, 1);
    EXPECT_EQ(with,    1);   // 최적화로도 줄지 않음
}

// ════════════════════════════════════════════════════
// Wave 5 — 조건문: 상수 condition 제거
// ════════════════════════════════════════════════════

// if (1 < 2) print "yes";
// 최적화 전: condition에 BinaryExpr 1개
// 최적화 후: condition이 LiteralExpr(true)로 접힘 → 0개
TEST(ShellConstantFoldingOptimizerTest, IfCondition_ConstantReduced) {
    EXPECT_EQ(binaryExprCountFor(R"(if (1 < 2) print "yes";)", false), 1);
    EXPECT_EQ(binaryExprCountFor(R"(if (1 < 2) print "yes";)", true),  0);
}

// ════════════════════════════════════════════════════
// Wave 6 — optimizer2.md 예제: 루프 내 상수 접기 (실행 포함)
// ════════════════════════════════════════════════════
//
// var total = 0;
// for (var i = 0; i < 100; i = i + 1) {
//     total = total + (1 - 2 * 3 * 4 * 5 / 6 + 7 + 8 + 9);
// }
//
// AST 내 BinaryExpr 분포 (최적화 전, 11개):
//   - 제어식: i < 100 (1개), i + 1 (1개)
//   - 본문  : total + (...) (1개)
//             + 상수 표현식 1-2*3*4*5/6+7+8+9 (8개: *, *, *, /, -, +, +, +)
//
// 최적화 후 (3개):
//   - 제어식: i < 100 (1개), i + 1 (1개)  ← 변수 포함으로 접기 불가, 유지
//   - 본문  : total + LiteralExpr(5) (1개) ← 상수 8개가 LiteralExpr(5) 1개로 교체
//
// 런타임 절감: 8개 × 100회 = 800번 BinaryExpr 평가 감소
TEST(ShellConstantFoldingOptimizerTest, LargeLoop_Optimizer2Example_ConstantFolded) {
    const std::string src =
        "var total = 0; "
        "for (var i = 0; i < 100; i = i + 1) { "
        "  total = total + (1 - 2 * 3 * 4 * 5 / 6 + 7 + 8 + 9); "
        "}";

    int without = binaryExprCountFor(src, false);
    int with    = binaryExprCountFor(src, true);

    EXPECT_EQ(without, 11);  // 제어식 2개 + 본문 1개 + 상수 표현식 8개
    EXPECT_EQ(with,     3);  // 제어식 2개 + 본문 1개 (상수 8개 → LiteralExpr(5))
    EXPECT_EQ(without - with, 8);  // 루프 1회당 8번 평가 절감
}

// ════════════════════════════════════════════════════
// Wave 7 — optimizer2.md 예제: 런타임 전체 평가 횟수 검증
// ════════════════════════════════════════════════════
//
// for 루프를 실제로 100회 실행하고 BinaryExpr.accept() 호출 횟수를 집계한다.
//
// 최적화 전 (100회 루프):
//   조건 i < 100   : 101회 (0~99 통과 + 100에서 탈출)
//   증감 i + 1     : 100회
//   본문 total + X : 100회
//   상수 표현식 8개 : 100회 × 8 = 800회
//   합계           : 101 + 100 + 100 + 800 = 1101회
//
// 최적화 후 (상수 8개 → LiteralExpr(5)):
//   조건 i < 100   : 101회
//   증감 i + 1     : 100회
//   본문 total + 5 : 100회
//   합계           : 101 + 100 + 100 = 301회
//
//   절감 횟수: 1101 - 301 = 800회 (= 8개 × 100회)
TEST(ShellConstantFoldingOptimizerTest, LargeLoop_RuntimeEvaluationCount_Verified) {
    const std::string src =
        "var total = 0; "
        "for (var i = 0; i < 100; i = i + 1) { "
        "  total = total + (1 - 2 * 3 * 4 * 5 / 6 + 7 + 8 + 9); "
        "}";

    int without = runtimeBinaryExprCountFor(src, false);
    int with    = runtimeBinaryExprCountFor(src, true);

    EXPECT_EQ(without, 1101);
    EXPECT_EQ(with,     301);
    EXPECT_EQ(without - with, 800);  // 상수 8개 × 100회 = 800번 절감
}
