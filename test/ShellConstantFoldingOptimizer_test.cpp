#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"

// ── Test Double ───────────────────────────────────────────────────────────
// RuntimeSpyBinaryExpr : accept() 호출마다 공유 카운터를 증가시킨다.
// RuntimeSpyOptimizer  : AST의 모든 BinaryExpr를 RuntimeSpyBinaryExpr로 교체한다.
//   ConstantFoldingOptimizer 이후에 체인에 추가하면, 접히고 남은 BinaryExpr만
//   카운트되어 루프 반복을 포함한 실제 런타임 평가 횟수를 측정할 수 있다.
namespace {

struct RuntimeSpyBinaryExpr : BinaryExpr {
    std::shared_ptr<int> callCount;
    RuntimeSpyBinaryExpr(ExprPtr l, Token op, ExprPtr r, std::shared_ptr<int> c)
        : BinaryExpr(std::move(l), std::move(op), std::move(r)), callCount(c) {}
    LiteralValue accept(ExprVisitor& v) override {
        ++(*callCount);
        return v.visitBinaryExpr(*this);
    }
};

struct RuntimeSpyOptimizer : IOptimizer {
    std::shared_ptr<int> callCount;
    explicit RuntimeSpyOptimizer(std::shared_ptr<int> c) : callCount(c) {}

    std::vector<StmtPtr> optimize(std::vector<StmtPtr> stmts) override {
        for (auto& s : stmts) wrapStmt(s);
        return std::move(stmts);
    }

private:
    ExprPtr wrapExpr(ExprPtr e) {
        if (!e) return e;
        if (auto* b = dynamic_cast<BinaryExpr*>(e.get())) {
            auto l = wrapExpr(std::move(b->left));
            auto r = wrapExpr(std::move(b->right));
            return std::make_unique<RuntimeSpyBinaryExpr>(
                std::move(l), b->op, std::move(r), callCount);
        }
        if (auto* g = dynamic_cast<GroupingExpr*>(e.get()))
            g->expression = wrapExpr(std::move(g->expression));
        else if (auto* u = dynamic_cast<UnaryExpr*>(e.get()))
            u->right = wrapExpr(std::move(u->right));
        else if (auto* a = dynamic_cast<AssignExpr*>(e.get()))
            a->value = wrapExpr(std::move(a->value));
        else if (auto* l = dynamic_cast<LogicalExpr*>(e.get())) {
            l->left  = wrapExpr(std::move(l->left));
            l->right = wrapExpr(std::move(l->right));
        }
        return e;
    }

    void wrapStmt(StmtPtr& s) {
        if (!s) return;
        if (auto* p = dynamic_cast<ExpressionStmt*>(s.get()))
            p->expression = wrapExpr(std::move(p->expression));
        else if (auto* p = dynamic_cast<PrintStmt*>(s.get()))
            p->expression = wrapExpr(std::move(p->expression));
        else if (auto* p = dynamic_cast<VarDeclareStmt*>(s.get()))
            p->initializer = wrapExpr(std::move(p->initializer));
        else if (auto* p = dynamic_cast<BlockStmt*>(s.get()))
            for (auto& inner : p->statements) wrapStmt(inner);
        else if (auto* p = dynamic_cast<IfStmt*>(s.get())) {
            p->condition = wrapExpr(std::move(p->condition));
            wrapStmt(p->thenBranch);
            wrapStmt(p->elseBranch);
        } else if (auto* p = dynamic_cast<ForStmt*>(s.get())) {
            wrapStmt(p->initializer);
            p->condition = wrapExpr(std::move(p->condition));
            p->increment = wrapExpr(std::move(p->increment));
            wrapStmt(p->body);
        } else if (auto* p = dynamic_cast<FunctionStmt*>(s.get())) {
            for (auto& inner : p->body) wrapStmt(inner);
        } else if (auto* p = dynamic_cast<ReturnStmt*>(s.get())) {
            if (p->value) p->value = wrapExpr(std::move(p->value));
        }
    }
};

} // namespace

// ── 헬퍼 ──────────────────────────────────────────────────────────────────
// [ConstantFoldingOptimizer →] RuntimeSpyOptimizer 순으로 체인을 구성하고
// 소스를 실행한 뒤 BinaryExpr.accept() 런타임 호출 횟수를 반환한다.
static int evalCountFor(const std::string& src, bool withOpt) {
    auto count = std::make_shared<int>(0);
    Shell shell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::make_unique<Executor>()
    );
    if (withOpt)
        shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    shell.addOptimizer(std::make_unique<RuntimeSpyOptimizer>(count));
    shell.runLine(src);
    return *count;
}

// ────────────────────────────────────────────────────────────────────────
// 단일 실행 테스트 (비루프): 런타임 횟수 == AST 노드 수
// ────────────────────────────────────────────────────────────────────────

// print 2 + 3;  →  1회 → 0회
TEST(ShellConstantFoldingOptimizerTest, SingleConstant_ReducedToZero) {
    EXPECT_EQ(evalCountFor("print 2 + 3;", false), 1);
    EXPECT_EQ(evalCountFor("print 2 + 3;", true),  0);
}

// print (2+3)*(4+5);  →  3회 → 0회
TEST(ShellConstantFoldingOptimizerTest, NestedConstant_AllReducedToZero) {
    EXPECT_EQ(evalCountFor("print (2 + 3) * (4 + 5);", false), 3);
    EXPECT_EQ(evalCountFor("print (2 + 3) * (4 + 5);", true),  0);
}

// var a = 5 * 4;  →  1회 → 0회
TEST(ShellConstantFoldingOptimizerTest, ConstantInitializer_ReducedToZero) {
    EXPECT_EQ(evalCountFor("var a = 5 * 4;", false), 1);
    EXPECT_EQ(evalCountFor("var a = 5 * 4;", true),  0);
}

// if (1 < 2) print "yes";  →  1회 → 0회
TEST(ShellConstantFoldingOptimizerTest, IfCondition_ConstantReduced) {
    EXPECT_EQ(evalCountFor(R"(if (1 < 2) print "yes";)", false), 1);
    EXPECT_EQ(evalCountFor(R"(if (1 < 2) print "yes";)", true),  0);
}

// var x=10; print x + 1*2;  →  2회(x+_, 1*2) → 1회(x+2만 남음)
TEST(ShellConstantFoldingOptimizerTest, MixedExpr_ConstantSubtreeRemoved) {
    EXPECT_EQ(evalCountFor("var x = 10; print x + 1 * 2;", false), 2);
    EXPECT_EQ(evalCountFor("var x = 10; print x + 1 * 2;", true),  1);
}

// var x=5; var y=3; print x+y;  →  최적화 전후 모두 1회 (변수 접기 불가)
TEST(ShellConstantFoldingOptimizerTest, VariableExpr_CountUnchanged) {
    EXPECT_EQ(evalCountFor("var x = 5; var y = 3; print x + y;", false), 1);
    EXPECT_EQ(evalCountFor("var x = 5; var y = 3; print x + y;", true),  1);
}

// ────────────────────────────────────────────────────────────────────────
// 루프 테스트: 런타임 횟수 = AST 노드 수 × 실행 횟수
// ────────────────────────────────────────────────────────────────────────

// for(var i=0; i<3; i=i+1) { print 2+3; }
//   최적화 전: 조건(4) + 증감(3) + 본문 2+3(3) = 10회
//   최적화 후: 조건(4) + 증감(3)               =  7회  (2+3 → LiteralExpr(5))
TEST(ShellConstantFoldingOptimizerTest, LoopBody_ConstantExprFolded) {
    EXPECT_EQ(evalCountFor("for(var i=0; i<3; i=i+1) { print 2+3; }", false), 10);
    EXPECT_EQ(evalCountFor("for(var i=0; i<3; i=i+1) { print 2+3; }", true),   7);
}

// print 10 % 3;  →  1회 → 0회 (LiteralExpr(1)로 접힘)
TEST(ShellConstantFoldingOptimizerTest, Modulo_ConstantFolded) {
    EXPECT_EQ(evalCountFor("print 10 % 3;", false), 1);
    EXPECT_EQ(evalCountFor("print 10 % 3;", true),  0);
}

// optimizer2.md 원본 예제 — 100회 루프, % 연산 포함 상수식 10개
//
//   본문: total = total + (1 - 2*3*4*5/6+7+8+9) % 1000 % 30
//   BinaryExpr 구성:
//     total + (...)            : 1개 (변수 포함, 유지)
//     GroupingExpr 내부 8개    : *, *, *, /, -, +, +, +
//     (GroupingExpr) % 1000    : 1개 (상수)
//     _ % 30                   : 1개 (상수)
//     → 상수 부분 10개가 LiteralExpr(5) 하나로 접힘
//
//   최적화 전: 조건(101) + 증감(100) + 본문 11개×100(1100) = 1301회
//   최적화 후: 조건(101) + 증감(100) + 본문   1개×100(100) =  301회
//   절감: 1000회 (10개 × 100회)
TEST(ShellConstantFoldingOptimizerTest, LargeLoop_RuntimeEvaluationCount_Verified) {
    const std::string src =
        "var total = 0; "
        "for (var i = 0; i < 100; i = i + 1) { "
        "  total = total + (1 - 2 * 3 * 4 * 5 / 6 + 7 + 8 + 9) % 1000 % 30; "
        "}";
    EXPECT_EQ(evalCountFor(src, false), 1301);
    EXPECT_EQ(evalCountFor(src, true),   301);
}

// print "Hello" + " World";  →  1회 → 0회 (문자열 PLUS 경로 검증)
TEST(ShellConstantFoldingOptimizerTest, StringConcatenation_ConstantFolded) {
    EXPECT_EQ(evalCountFor(R"(print "Hello" + " World";)", false), 1);
    EXPECT_EQ(evalCountFor(R"(print "Hello" + " World";)", true),  0);
}

// print 3 > 2;  →  1회 → 0회, 비교 연산 결과(true) LiteralExpr로 접힘
TEST(ShellConstantFoldingOptimizerTest, ComparisonOperator_ConstantFolded) {
    EXPECT_EQ(evalCountFor("print 3 > 2;", false), 1);
    EXPECT_EQ(evalCountFor("print 3 > 2;", true),  0);
}

// Func f() { return 2 + 3; } print f();
//   최적화 전: return 내 2+3 런타임 1회 평가
//   최적화 후: return 내 LiteralExpr(5)로 교체 → 0회
TEST(ShellConstantFoldingOptimizerTest, FunctionBody_ConstantFolded) {
    EXPECT_EQ(evalCountFor("Func f() { return 2 + 3; } print f();", false), 1);
    EXPECT_EQ(evalCountFor("Func f() { return 2 + 3; } print f();", true),  0);
}
