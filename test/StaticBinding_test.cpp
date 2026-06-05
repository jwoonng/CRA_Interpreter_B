#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/optimizer/StaticBindingOptimizer.h"
#include "src/executor/IExecutor.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/shell/Shell.h"

using ::testing::_;
using ::testing::Invoke;

// ── Test Double: Mock (gmock) ──────────────────────────────────────────────
// execute() 에 전달된 stmts 를 lambda action 으로 검사한다.
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

static Token identTok(const std::string& name, int line = 1) {
    return makeTok(TokenType::IDENTIFIER, name, line);
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 1: StaticBindingOptimizerFixture
// Optimizer 가 AST 노드의 distance 필드를 올바르게 설정하는지 직접 검증한다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingOptimizerFixture : public ::testing::Test {
protected:
    StaticBindingOptimizer optimizer_;
};

// { var x = 1; print x; }  →  x.distance == 0 (같은 블록 스코프)
TEST_F(StaticBindingOptimizerFixture, SetsDistanceZeroForSameScope) {
    auto varTok  = identTok("x");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> block;
    block.push_back(std::make_unique<VarDeclareStmt>(varTok));
    block.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 1));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(block)));

    optimizer_.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, 0);
}

// { var a = 0; { print a; } }  →  a.distance == 1 (바깥 블록 스코프)
TEST_F(StaticBindingOptimizerFixture, SetsDistanceOneForOuterScope) {
    auto varTok  = identTok("a");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> innerBlock;
    innerBlock.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 2));

    std::vector<StmtPtr> outerBlock;
    outerBlock.push_back(std::make_unique<VarDeclareStmt>(varTok));
    outerBlock.push_back(std::make_unique<BlockStmt>(std::move(innerBlock)));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outerBlock)));

    optimizer_.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, 1);
}

// var g = 0; print g;  →  g.distance == -1 (글로벌: scopes_ 비어있어 미설정)
TEST_F(StaticBindingOptimizerFixture, GlobalVariableDistanceRemainsMinusOne) {
    auto varTok  = identTok("g");
    auto varExpr = std::make_unique<VariableExpr>(varTok);
    VariableExpr* raw = varExpr.get();

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(varTok));
    stmts.push_back(std::make_unique<PrintStmt>(std::move(varExpr), 2));

    optimizer_.optimize(std::move(stmts));

    EXPECT_EQ(raw->distance, -1);
}

// { var x = 0; { x = 5; } }  →  AssignExpr.distance == 1
TEST_F(StaticBindingOptimizerFixture, AssignExprDistanceSetForOuterScope) {
    auto varTok   = identTok("x");
    auto assignEx = std::make_unique<AssignExpr>(varTok,
        std::make_unique<LiteralExpr>(5.0, 2));
    AssignExpr* rawA = assignEx.get();

    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<ExpressionStmt>(std::move(assignEx)));

    std::vector<StmtPtr> outer;
    outer.push_back(std::make_unique<VarDeclareStmt>(varTok));
    outer.push_back(std::make_unique<BlockStmt>(std::move(inner)));

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outer)));

    optimizer_.optimize(std::move(stmts));

    EXPECT_EQ(rawA->distance, 1);
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 2: StaticBindingMockFixture
// Test Double — Mock (gmock MockExecutor)
//
// Optimizer 통과 후 Executor 가 받는 AST 의 VariableExpr.distance 를 검증한다.
// distance >= 0 이면 Executor 는 getAt() 으로 직접 접근하므로,
// 스코프를 거슬러 올라가지 않고 계산된 위치로 즉시 접근함이 보장된다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingMockFixture : public ::testing::Test {
protected:
    MockExecutor* mockPtr_ = nullptr;
    std::unique_ptr<Shell> shell_;

    void SetUp() override {
        auto mock  = std::make_unique<MockExecutor>();
        mockPtr_   = mock.get();
        shell_     = std::make_unique<Shell>(
            std::make_unique<Tokenizer>(),
            std::make_unique<Parser>(),
            std::make_unique<Checker>(),
            std::move(mock)
        );
        shell_->addOptimizer(std::make_unique<StaticBindingOptimizer>());
    }
};

// { var x = 1; print x; }  →  Executor 가 받는 VariableExpr.distance == 0
// distance >= 0 이므로 Executor 는 getAt(0, "x") 로 즉시 접근한다.
TEST_F(StaticBindingMockFixture, LocalVarRef_DistanceSetToZero_ImpliesGetAt) {
    EXPECT_CALL(*mockPtr_, execute(_, _))
        .WillOnce([](const std::vector<std::unique_ptr<Stmt>>& stmts,
                     std::ostream&) {
            ASSERT_FALSE(stmts.empty());
            auto* block = dynamic_cast<BlockStmt*>(stmts[0].get());
            ASSERT_NE(block, nullptr);
            ASSERT_GE(block->statements.size(), 2u);

            auto* ps = dynamic_cast<PrintStmt*>(block->statements[1].get());
            ASSERT_NE(ps, nullptr);
            auto* ve = dynamic_cast<VariableExpr*>(ps->expression.get());
            ASSERT_NE(ve, nullptr);

            // distance >= 0 → Executor::visitVariableExpr 이 getAt() 호출
            EXPECT_GE(ve->distance, 0)
                << "Optimizer 적용 시 distance 가 설정되어 getAt() 으로 직접 접근해야 합니다.";
        });

    shell_->runLine("{ var x = 1; print x; }");
}

// 복수 변수, 다양한 스코프 깊이: 모든 VariableExpr 의 distance >= 0 검증
// { var a = 1; var b = 2; { { print a; print b; } } }
// a: distance == 2,  b: distance == 2
TEST_F(StaticBindingMockFixture, AllLocalVarRefs_HaveNonNegativeDistance) {
    EXPECT_CALL(*mockPtr_, execute(_, _))
        .WillOnce([](const std::vector<std::unique_ptr<Stmt>>& stmts,
                     std::ostream&) {
            ASSERT_FALSE(stmts.empty());
            auto* outer = dynamic_cast<BlockStmt*>(stmts[0].get());
            ASSERT_NE(outer, nullptr);
            ASSERT_GE(outer->statements.size(), 3u);

            // statements[2] = 중간 블록
            auto* mid = dynamic_cast<BlockStmt*>(outer->statements[2].get());
            ASSERT_NE(mid, nullptr);
            ASSERT_GE(mid->statements.size(), 1u);

            // statements[0] = 안쪽 블록
            auto* inner = dynamic_cast<BlockStmt*>(mid->statements[0].get());
            ASSERT_NE(inner, nullptr);

            for (const auto& stmt : inner->statements) {
                auto* ps = dynamic_cast<PrintStmt*>(stmt.get());
                ASSERT_NE(ps, nullptr);
                auto* ve = dynamic_cast<VariableExpr*>(ps->expression.get());
                ASSERT_NE(ve, nullptr);
                EXPECT_GE(ve->distance, 0)
                    << "모든 로컬 변수 참조는 distance >= 0 이어야 합니다.";
            }
        });

    shell_->runLine("{ var a = 1; var b = 2; { { print a; print b; } } }");
}

// 옵티마이저 없이 실행 시 distance == -1 유지 확인
// distance == -1 이면 Executor 는 get() 으로 동적 탐색한다.
TEST_F(StaticBindingMockFixture, WithoutOptimizer_DistanceIsMinusOne_ImpliesGet) {
    // 옵티마이저 없는 별도 Shell
    auto mock2 = std::make_unique<MockExecutor>();
    MockExecutor* raw2 = mock2.get();
    Shell plainShell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::move(mock2)
    );

    EXPECT_CALL(*raw2, execute(_, _))
        .WillOnce([](const std::vector<std::unique_ptr<Stmt>>& stmts,
                     std::ostream&) {
            if (stmts.empty()) return;
            auto* block = dynamic_cast<BlockStmt*>(stmts[0].get());
            if (!block || block->statements.size() < 2) return;
            auto* ps = dynamic_cast<PrintStmt*>(block->statements[1].get());
            if (!ps) return;
            auto* ve = dynamic_cast<VariableExpr*>(ps->expression.get());
            if (!ve) return;

            // distance == -1 → Executor::visitVariableExpr 이 get() 호출 (동적 탐색)
            EXPECT_EQ(ve->distance, -1)
                << "Optimizer 미적용 시 distance 는 -1 이어야 합니다.";
        });

    plainShell.runLine("{ var x = 1; print x; }");
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 3: StaticBindingIntegrationFixture
// 최적화 적용 후 실제 실행 결과의 정확성을 검증한다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingIntegrationFixture : public ::testing::Test {
protected:
    Shell shell_;

    void SetUp() override {
        shell_.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    }
};

// { var a = 42; { { print a; } } }  →  "42\n"
TEST_F(StaticBindingIntegrationFixture, NestedScopeVariableRead_ReturnsCorrectValue) {
    EXPECT_EQ(shell_.runLine("{ var a = 42; { { print a; } } }"), "42\n");
}

// 글로벌 변수 → 중첩 스코프 대입 → 글로벌 변수 출력
TEST_F(StaticBindingIntegrationFixture, GlobalVarAssignedInNestedScope_ReflectsChange) {
    shell_.runLine("var x = 1;");
    shell_.runLine("{ { x = 99; } }");
    EXPECT_EQ(shell_.runLine("print x;"), "99\n");
}

// 동일 이름의 로컬 변수가 글로벌을 가리지 않아야 한다 (Shadowing)
TEST_F(StaticBindingIntegrationFixture, LocalShadowsGlobal_OuterUnaffected) {
    shell_.runLine("var x = 10;");
    EXPECT_EQ(shell_.runLine("{ var x = 99; print x; }"), "99\n");
    EXPECT_EQ(shell_.runLine("print x;"), "10\n");
}
