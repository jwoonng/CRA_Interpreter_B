#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/optimizer/StaticBindingOptimizer.h"
#include "src/executor/Executor.h"
#include "src/executor/IExecutor.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/shell/Shell.h"

using ::testing::_;

// ════════════════════════════════════════════════════════════════════════════
// Test Double — Mock (gmock)
// execute() 에 전달된 stmts 의 VariableExpr.distance 가 설정됐는지 검증한다.
// ════════════════════════════════════════════════════════════════════════════
class MockExecutor : public IExecutor {
public:
    MOCK_METHOD(void, execute,
        (const std::vector<std::unique_ptr<Stmt>>&, std::ostream&),
        (override));
};

// ════════════════════════════════════════════════════════════════════════════
// 헬퍼
// ════════════════════════════════════════════════════════════════════════════
static Token makeTok(TokenType type, const std::string& lex, int line = 1) {
    return Token{ type, lex, std::monostate{}, line };
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 1: StaticBindingOptimizerFixture
// Optimizer가 AST 노드의 distance 필드를 올바르게 설정하는지 직접 검증한다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingOptimizerFixture : public ::testing::Test {
protected:
    StaticBindingOptimizer optimizer_;

    // 테스트 편의를 위한 토큰 생성 헬퍼
    static Token tok(const std::string& name, int line = 1) {
        return makeTok(TokenType::IDENTIFIER, name, line);
    }
};

// { var x = 1; print x; }  →  x.distance == 0 (같은 블록 스코프)
TEST_F(StaticBindingOptimizerFixture, SetsDistanceZeroForSameScope) {
    auto varTok  = tok("x");
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
    auto varTok  = tok("a");
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
    auto varTok  = tok("g");
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
    auto varTok   = tok("x");
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
// Fixture 2: StaticBindingSpyFixture
// Test Double — Spy (Environment::LookupSpy)
//
// 핵심 검증:
//   최적화 적용 시 get() (스코프 체인 순회) 대신
//   getAt() (계산된 위치로 즉시 접근) 가 호출되는지 확인한다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingSpyFixture : public ::testing::Test {
    // 각 TC 는 LookupSpyGuard 를 직접 생성하여 spy 활성화/비활성화를 제어한다.
};

// 옵티마이저 적용 → getAt() 호출, get() 미호출 (스코프 탐색 없음)
// 프로그램: { var x = 1; { { print x; } } }  (x: distance = 2)
TEST_F(StaticBindingSpyFixture, WithOptimizer_LocalVarRead_UsesGetAt_NotGet) {
    Environment::LookupSpyGuard guard;

    Shell shell;
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    shell.runLine("{ var x = 1; { { print x; } } }");

    EXPECT_GT(guard.spy.getAtCount, 0)
        << "StaticBindingOptimizer 적용 시 getAt() 이 호출되어야 합니다.";
    EXPECT_EQ(guard.spy.getCount, 0)
        << "StaticBindingOptimizer 적용 시 로컬 변수에 대해 get() 은 호출되지 않아야 합니다.";
}

// 옵티마이저 미적용 → get() 호출 (스코프 체인 순회), getAt() 미호출
TEST_F(StaticBindingSpyFixture, WithoutOptimizer_LocalVarRead_UsesGet_NotGetAt) {
    Environment::LookupSpyGuard guard;

    Shell shell;   // optimizer 없음
    shell.runLine("{ var x = 1; { { print x; } } }");

    EXPECT_EQ(guard.spy.getAtCount, 0)
        << "옵티마이저 없이는 getAt() 이 호출되지 않아야 합니다.";
    EXPECT_GT(guard.spy.getCount, 0)
        << "옵티마이저 없이는 get() 이 스코프 탐색에 호출되어야 합니다.";
}

// 옵티마이저 적용 시, get() 호출 수 < 미적용 시 get() 호출 수
// 깊게 중첩된 프로그램에서 탐색 횟수가 실제로 줄어드는지 검증한다.
TEST_F(StaticBindingSpyFixture, WithOptimizer_DeeplyNested_FewerLookupsThanWithout) {
    int getCountWithOptimizer = 0;
    int getCountWithout       = 0;

    {
        Environment::LookupSpyGuard guard;
        Shell shell;
        shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
        shell.runLine("{ var a = 0; { { { { { a = a + 1; } } } } } }");
        getCountWithOptimizer = guard.spy.getCount;
    }
    {
        Environment::LookupSpyGuard guard;
        Shell shell;
        shell.runLine("{ var a = 0; { { { { { a = a + 1; } } } } } }");
        getCountWithout = guard.spy.getCount;
    }

    EXPECT_LT(getCountWithOptimizer, getCountWithout)
        << "최적화 적용 시 get() 호출이 감소해야 합니다.";
}

// 여러 로컬 변수 읽기 → 각 참조마다 getAt() 이 호출되어야 한다.
TEST_F(StaticBindingSpyFixture, WithOptimizer_MultipleVarReads_EachCallsGetAt) {
    Environment::LookupSpyGuard guard;

    Shell shell;
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    // a, b, c 각각 1번씩 읽기 — 총 getAt 3회 기대
    shell.runLine("{ var a = 1; var b = 2; var c = 3; print a; print b; print c; }");

    EXPECT_EQ(guard.spy.getAtCount, 3)
        << "로컬 변수 3개 읽기에 대해 getAt() 이 3번 호출되어야 합니다.";
    EXPECT_EQ(guard.spy.getCount, 0)
        << "최적화 적용 시 get() 은 호출되지 않아야 합니다.";
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 3: StaticBindingMockFixture
// Test Double — Mock (gmock MockExecutor)
//
// Executor 가 받는 stmts 의 VariableExpr.distance 가
// 옵티마이저 통과 후 올바르게 설정됐는지 검증한다.
// ════════════════════════════════════════════════════════════════════════════
class StaticBindingMockFixture : public ::testing::Test {
protected:
    MockExecutor* mockPtr_ = nullptr;
    std::unique_ptr<Shell> shell_;

    void SetUp() override {
        auto mock = std::make_unique<MockExecutor>();
        mockPtr_  = mock.get();
        shell_    = std::make_unique<Shell>(
            std::make_unique<Tokenizer>(),
            std::make_unique<Parser>(),
            std::make_unique<Checker>(),
            std::move(mock)
        );
        shell_->addOptimizer(std::make_unique<StaticBindingOptimizer>());
    }
};

// { var x = 1; print x; } → Executor 가 받는 VariableExpr.distance == 0
TEST_F(StaticBindingMockFixture, ExecutorReceivesAstWithDistanceSet) {
    EXPECT_CALL(*mockPtr_, execute(_, _))
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

            EXPECT_EQ(ve->distance, 0);   // 같은 블록 스코프 → distance = 0
        });

    shell_->runLine("{ var x = 1; print x; }");
}

// 옵티마이저 없이 실행 시 Executor 가 받는 VariableExpr.distance == -1
TEST_F(StaticBindingMockFixture, WithoutOptimizer_DistanceRemainsMinusOne) {
    // 옵티마이저 없는 별도 Shell 생성
    auto mock2 = std::make_unique<MockExecutor>();
    MockExecutor* raw2 = mock2.get();
    Shell plainShell(
        std::make_unique<Tokenizer>(),
        std::make_unique<Parser>(),
        std::make_unique<Checker>(),
        std::move(mock2)
    );
    // addOptimizer 호출 없음

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

            EXPECT_EQ(ve->distance, -1);  // 미최적화: 기본값 유지
        });

    plainShell.runLine("{ var x = 1; print x; }");
}

// ════════════════════════════════════════════════════════════════════════════
// Fixture 4: StaticBindingIntegrationFixture
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

// 전역 변수 → 중첩 스코프 대입 → 전역 변수 출력
TEST_F(StaticBindingIntegrationFixture, GlobalVarAssignedInNestedScope_ReflectsChange) {
    shell_.runLine("var x = 1;");
    shell_.runLine("{ { x = 99; } }");
    EXPECT_EQ(shell_.runLine("print x;"), "99\n");
}

// 동일 이름의 로컬 변수가 글로벌을 가리지 않아야 한다 (Shadowing)
TEST_F(StaticBindingIntegrationFixture, LocalShadowsGlobal_OuterUnaffected) {
    shell_.runLine("var x = 10;");
    EXPECT_EQ(shell_.runLine("{ var x = 99; print x; }"), "99\n");
    EXPECT_EQ(shell_.runLine("print x;"), "10\n");  // 글로벌은 변경 없음
}
