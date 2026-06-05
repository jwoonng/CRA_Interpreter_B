#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "src/common/Expr.h"
#include "src/common/Stmt.h"
#include "src/executor/Executor.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"

// ── Test Double: SpyBinaryExpr ────────────────────────────────────────────
// accept()가 호출될 때마다 공유 카운터를 증가시켜 런타임 평가 횟수를 추적한다.
struct SpyBinaryExpr : BinaryExpr {
    std::shared_ptr<int> callCount;

    SpyBinaryExpr(ExprPtr left, Token op, ExprPtr right, std::shared_ptr<int> count)
        : BinaryExpr(std::move(left), std::move(op), std::move(right))
        , callCount(std::move(count)) {}

    LiteralValue accept(ExprVisitor& v) override {
        ++(*callCount);
        return v.visitBinaryExpr(*this);
    }
};

// ── 헬퍼 ──────────────────────────────────────────────────────────────────
static Token tok(TokenType type, std::string lexeme,
                 LiteralValue lit = std::monostate{}, int line = 1) {
    return Token{ type, std::move(lexeme), std::move(lit), line };
}

// print (2 * 3) + 4;  →  "10\n"
// SpyBinaryExpr 2개: 곱셈 노드, 덧셈 노드
static std::vector<StmtPtr> makeSpyPrintStmt(std::shared_ptr<int> count) {
    auto mul = std::make_unique<SpyBinaryExpr>(
        std::make_unique<LiteralExpr>(2.0, 1),
        tok(TokenType::STAR, "*"),
        std::make_unique<LiteralExpr>(3.0, 1),
        count
    );
    auto add = std::make_unique<SpyBinaryExpr>(
        std::move(mul),
        tok(TokenType::PLUS, "+"),
        std::make_unique<LiteralExpr>(4.0, 1),
        count
    );
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<PrintStmt>(std::move(add), 1));
    return stmts;
}

// for (var i = 0; i < N; i = i + 1) { print (2 * 3) + 4; }
static std::vector<StmtPtr> makeSpyForStmt(int loopCount, std::shared_ptr<int> count) {
    auto initStmt = std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "i"),
        std::make_unique<LiteralExpr>(0.0, 1)
    );
    auto cond = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "i")),
        tok(TokenType::LESS, "<"),
        std::make_unique<LiteralExpr>(static_cast<double>(loopCount), 1)
    );
    auto incr = std::make_unique<AssignExpr>(
        tok(TokenType::IDENTIFIER, "i"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "i")),
            tok(TokenType::PLUS, "+"),
            std::make_unique<LiteralExpr>(1.0, 1)
        )
    );

    auto mul = std::make_unique<SpyBinaryExpr>(
        std::make_unique<LiteralExpr>(2.0, 1),
        tok(TokenType::STAR, "*"),
        std::make_unique<LiteralExpr>(3.0, 1),
        count
    );
    auto add = std::make_unique<SpyBinaryExpr>(
        std::move(mul),
        tok(TokenType::PLUS, "+"),
        std::make_unique<LiteralExpr>(4.0, 1),
        count
    );
    auto body = std::make_unique<PrintStmt>(std::move(add), 1);

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<ForStmt>(
        std::move(initStmt),
        std::move(cond),
        std::move(incr),
        std::move(body)
    ));
    return stmts;
}

// ── 테스트 케이스 ─────────────────────────────────────────────────────────

// 최적화 없이 실행하면 BinaryExpr가 N회 평가된다 (단일 print 문, N=2)
TEST(ConstantFoldingOptimizerTest, NoOptimization_EvaluatesNTimes) {
    auto count = std::make_shared<int>(0);
    auto stmts = makeSpyPrintStmt(count);

    Executor ex;
    std::ostringstream oss;
    ex.execute(stmts, oss);

    EXPECT_EQ(*count, 2);         // 곱셈 1회 + 덧셈 1회
    EXPECT_EQ(oss.str(), "10\n");
}

// 최적화 후 실행하면 SpyBinaryExpr가 LiteralExpr로 교체되어 평가 횟수 = 0
TEST(ConstantFoldingOptimizerTest, WithOptimization_NotEvaluatedAtRuntime) {
    auto count = std::make_shared<int>(0);
    auto stmts = makeSpyPrintStmt(count);

    ConstantFoldingOptimizer opt;
    auto optimized = opt.optimize(std::move(stmts));

    Executor ex;
    std::ostringstream oss;
    ex.execute(optimized, oss);

    EXPECT_EQ(*count, 0);         // Spy 노드가 LiteralExpr(10)으로 교체됨
    EXPECT_EQ(oss.str(), "10\n");
}

// 루프 3회 반복, 최적화 없음: 2 ops × 3 = 6회 평가
TEST(ConstantFoldingOptimizerTest, LoopNoOptimization_EvaluatesNTimesPerIteration) {
    auto count = std::make_shared<int>(0);
    auto stmts = makeSpyForStmt(3, count);

    Executor ex;
    std::ostringstream oss;
    ex.execute(stmts, oss);

    EXPECT_EQ(*count, 6);         // 루프 3회 × BinaryExpr 2개
    EXPECT_EQ(oss.str(), "10\n10\n10\n");
}

// 루프 3회 반복, 최적화 후: 루프 본문의 상수 식이 LiteralExpr로 교체 → 0회
TEST(ConstantFoldingOptimizerTest, LoopWithOptimization_FoldedToZeroEvaluations) {
    auto count = std::make_shared<int>(0);
    auto stmts = makeSpyForStmt(3, count);

    ConstantFoldingOptimizer opt;
    auto optimized = opt.optimize(std::move(stmts));

    Executor ex;
    std::ostringstream oss;
    ex.execute(optimized, oss);

    EXPECT_EQ(*count, 0);         // 루프 반복 횟수와 무관하게 0
    EXPECT_EQ(oss.str(), "10\n10\n10\n");
}

// print 10 % 3;  →  SpyBinaryExpr(%) 최적화 전 1회, 최적화 후 0회 (LiteralExpr(1))
TEST(ConstantFoldingOptimizerTest, Modulo_FoldedToZeroEvaluations) {
    auto count = std::make_shared<int>(0);

    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<SpyBinaryExpr>(
            std::make_unique<LiteralExpr>(10.0, 1),
            tok(TokenType::PERCENT, "%"),
            std::make_unique<LiteralExpr>(3.0, 1),
            count
        ), 1
    ));

    ConstantFoldingOptimizer opt;
    auto optimized = opt.optimize(std::move(stmts));

    Executor ex;
    std::ostringstream oss;
    ex.execute(optimized, oss);

    EXPECT_EQ(*count, 0);          // SpyBinaryExpr가 LiteralExpr(1)로 교체됨
    EXPECT_EQ(oss.str(), "1\n");
}
