#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/optimizer/StaticBindingOptimizer.h"
#include "src/executor/IExecutor.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/shell/Shell.h"

using ::testing::_;

// ── MockExecutor ──────────────────────────────────────────────────────────
// execute() 호출 시 전달되는 stmts를 람다 액션으로 검사한다.
class MockExecutor : public IExecutor {
public:
    MOCK_METHOD(void, execute,
        (const std::vector<std::unique_ptr<Stmt>>&, std::ostream&),
        (override));
};

// ── 헬퍼 ──────────────────────────────────────────────────────────────────
static Token makeTok(TokenType type, const std::string& lex, int line = 1) {
    return Token{ type, lex, std::monostate{}, line };
}

// ══════════════════════════════════════════════════════════════════════════
// 1. 옵티마이저 단위 테스트 — AST 노드의 distance 필드 직접 검증
// ══════════════════════════════════════════════════════════════════════════

// { var x = 1; print x; }  →  x.distance == 0 (같은 블록 스코프)
TEST(StaticBindingOptimizerTest, SetsDistanceZeroForSameScope) {
    auto varTok  = makeTok(TokenType::IDENTIFIER, "x");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> block;
    block.push_back(std::make_unique<VarDeclareStmt>(varTok));
    block.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 1));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(block)));

    auto result = StaticBindingOptimizer{}.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, 0);
}

// { var a = 0; { print a; } }  →  a.distance == 1 (바깥 블록 스코프)
TEST(StaticBindingOptimizerTest, SetsDistanceOneForOuterScope) {
    auto varTok  = makeTok(TokenType::IDENTIFIER, "a");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> innerBlock;
    innerBlock.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 2));

    std::vector<StmtPtr> outerBlock;
    outerBlock.push_back(std::make_unique<VarDeclareStmt>(varTok));
    outerBlock.push_back(std::make_unique<BlockStmt>(std::move(innerBlock)));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outerBlock)));

    auto result = StaticBindingOptimizer{}.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, 1);
}

// var g = 0; print g;  →  g.distance == -1 (글로벌: scopes_ 비어있어 미설정)
TEST(StaticBindingOptimizerTest, GlobalVariableDistanceRemainsMinusOne) {
    auto varTok  = makeTok(TokenType::IDENTIFIER, "g");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(varTok));
    stmts.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 2));

    auto result = StaticBindingOptimizer{}.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, -1);
}

// { var x = 0; { x = 5; } }  →  AssignExpr.distance == 1
TEST(StaticBindingOptimizerTest, AssignExprGetsDistanceSet) {
    auto varTok   = makeTok(TokenType::IDENTIFIER, "x");
    auto assignEx = std::make_unique<AssignExpr>(
        varTok,
        std::make_unique<LiteralExpr>(5.0, 2)
    );
    AssignExpr* rawA = assignEx.get();

    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<ExpressionStmt>(std::move(assignEx)));

    std::vector<StmtPtr> outer;
    outer.push_back(std::make_unique<VarDeclareStmt>(varTok));
    outer.push_back(std::make_unique<BlockStmt>(std::move(inner)));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outer)));

    auto result = StaticBindingOptimizer{}.optimize(std::move(stmts));

    EXPECT_EQ(rawA->distance, 1);
}

// ══════════════════════════════════════════════════════════════════════════
// 2. MockExecutor 테스트 (gmock)
//    execute() 에 전달된 stmts 의 VariableExpr.distance 가 설정됐는지 검증
// ══════════════════════════════════════════════════════════════════════════

// { var x = 1; print x; }  →  Executor가 받는 VariableExpr.distance == 0
TEST(StaticBindingMockTest, ExecutorReceivesAstWithDistanceSet) {
    auto mockExec = std::make_unique<MockExecutor>();
    MockExecutor* raw = mockExec.get();

    Shell shell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::move(mockExec)
    );
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());

    EXPECT_CALL(*raw, execute(_, _))
        .WillOnce([](const std::vector<std::unique_ptr<Stmt>>& stmts,
                     std::ostream&) {
            EXPECT_FALSE(stmts.empty());
            if (stmts.empty()) return;

            auto* block = dynamic_cast<BlockStmt*>(stmts[0].get());
            EXPECT_NE(block, nullptr);
            if (!block || block->statements.size() < 2) return;

            auto* ps = dynamic_cast<PrintStmt*>(block->statements[1].get());
            EXPECT_NE(ps, nullptr);
            if (!ps) return;

            auto* ve = dynamic_cast<VariableExpr*>(ps->expression.get());
            EXPECT_NE(ve, nullptr);
            if (!ve) return;

            EXPECT_EQ(ve->distance, 0);   // 최적화 완료: 같은 스코프 distance = 0
        });

    shell.runLine("{ var x = 1; print x; }");
}

// 옵티마이저 없이 실행하면 distance 가 -1 이어야 한다
TEST(StaticBindingMockTest, WithoutOptimizerDistanceIsMinusOne) {
    auto mockExec = std::make_unique<MockExecutor>();
    MockExecutor* raw = mockExec.get();

    Shell shell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::move(mockExec)
        // addOptimizer 호출 없음
    );

    EXPECT_CALL(*raw, execute(_, _))
        .WillOnce([](const std::vector<std::unique_ptr<Stmt>>& stmts,
                     std::ostream&) {
            if (stmts.empty()) return;
            auto* block = dynamic_cast<BlockStmt*>(stmts[0].get());
            if (!block || block->statements.size() < 2) return;
            auto* ps = dynamic_cast<PrintStmt*>(block->statements[1].get());
            if (!ps) return;
            auto* ve = dynamic_cast<VariableExpr*>(ps->expression.get());
            if (!ve) return;

            EXPECT_EQ(ve->distance, -1);  // 미최적화: 기본값 -1
        });

    shell.runLine("{ var x = 1; print x; }");
}

// ══════════════════════════════════════════════════════════════════════════
// 3. Shell 통합 테스트 — 최적화 적용 후 올바른 실행 결과 검증
// ══════════════════════════════════════════════════════════════════════════

// { var a = 42; { { print a; } } }  →  "42\n"
TEST(StaticBindingIntegrationTest, NestedScopeVariableAccessCorrect) {
    Shell shell;
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    EXPECT_EQ(shell.runLine("{ var a = 42; { { print a; } } }"), "42\n");
}

// var x = 1; → { { x = 99; } } → print x;  →  "99\n"
TEST(StaticBindingIntegrationTest, AssignmentInNestedScopeCorrect) {
    Shell shell;
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    shell.runLine("var x = 1;");
    shell.runLine("{ { x = 99; } }");
    EXPECT_EQ(shell.runLine("print x;"), "99\n");
}
