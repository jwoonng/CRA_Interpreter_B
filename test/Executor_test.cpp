#include <gtest/gtest.h>
#include <sstream>
#include "src/executor/Executor.h"
#include "src/common/Expr.h"
#include "src/common/Stmt.h"

// ── Fixture ─────────────────────────────────────────────────────
class ExecutorTest : public ::testing::Test {
protected:
    Executor ex;
    std::ostringstream oss;
    std::vector<StmtPtr> stmts;

    std::string runStmts() {
        ex.execute(stmts, oss);
        return oss.str();
    }

    static Token tok(TokenType type, std::string lexeme,
                     LiteralValue lit = std::monostate{}, int line = 1) {
        return Token{ type, std::move(lexeme), std::move(lit), line };
    }
};

// ── 개발자 4 담당 영역 ──────────────────────────────────────────

// print 42;  →  "42"
TEST_F(ExecutorTest, PrintLiteral) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<LiteralExpr>(42.0, 1), 1
    ));
    EXPECT_EQ(runStmts(), "42\n");
}

// print 1 + 2 * 3;  →  "7\n"  (곱셈 우선)
TEST_F(ExecutorTest, ArithmeticPrecedence) {
    auto mul = std::make_unique<BinaryExpr>(
        std::make_unique<LiteralExpr>(2.0, 1),
        tok(TokenType::STAR, "*"),
        std::make_unique<LiteralExpr>(3.0, 1)
    );
    auto add = std::make_unique<BinaryExpr>(
        std::make_unique<LiteralExpr>(1.0, 1),
        tok(TokenType::PLUS, "+"),
        std::move(mul)
    );
    stmts.push_back(std::make_unique<PrintStmt>(std::move(add), 1));
    EXPECT_EQ(runStmts(), "7\n");
}

// var a = 10; var b = 20; print a + b;  →  "30"
TEST_F(ExecutorTest, VarDeclAndUse) {
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "a"),
        std::make_unique<LiteralExpr>(10.0, 1)
    ));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "b"),
        std::make_unique<LiteralExpr>(20.0, 1)
    ));
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "a")),
            tok(TokenType::PLUS, "+"),
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "b"))
        ), 1
    ));
    EXPECT_EQ(runStmts(), "30\n");
}

TEST_F(ExecutorTest, BlockScopeAndShadowing) {
    // TODO
}

TEST_F(ExecutorTest, IfTrue) {
    // TODO
}

TEST_F(ExecutorTest, IfFalseElse) {
    // TODO
}

TEST_F(ExecutorTest, ForLoop) {
    // TODO
}

TEST_F(ExecutorTest, UndefinedVariableThrows) {
    // TODO
}

TEST_F(ExecutorTest, DivisionByZeroThrows) {
    // TODO
}

TEST_F(ExecutorTest, TypeMismatchThrows) {
    // TODO
}
