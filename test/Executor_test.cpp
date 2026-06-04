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

// var x = "global"; { var x = "inner"; print x; } print x;
// →  "inner\nglobal\n"
TEST_F(ExecutorTest, BlockScopeAndShadowing) {
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "x"),
        std::make_unique<LiteralExpr>(std::string("global"), 1)
    ));

    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "x", std::monostate{}, 2),
        std::make_unique<LiteralExpr>(std::string("inner"), 2)
    ));
    inner.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "x", std::monostate{}, 2)),
        2
    ));
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));

    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "x", std::monostate{}, 3)),
        3
    ));
    EXPECT_EQ(runStmts(), "inner\nglobal\n");
}

// if (true) print "bbq";  →  "bbq\n"
TEST_F(ExecutorTest, IfTrue) {
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(true, 1),
        std::make_unique<PrintStmt>(
            std::make_unique<LiteralExpr>(std::string("bbq"), 1), 1
        )
    ));
    EXPECT_EQ(runStmts(), "bbq\n");
}

// if (false) print "no"; else print "kfc";  →  "kfc\n"
TEST_F(ExecutorTest, IfFalseElse) {
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(false, 1),
        std::make_unique<PrintStmt>(
            std::make_unique<LiteralExpr>(std::string("no"), 1), 1
        ),
        std::make_unique<PrintStmt>(
            std::make_unique<LiteralExpr>(std::string("kfc"), 1), 1
        )
    ));
    EXPECT_EQ(runStmts(), "kfc\n");
}

// for (var j = 0; j < 3; j = j + 1) { print j; }  →  "0\n1\n2\n"
TEST_F(ExecutorTest, ForLoop) {
    auto init = std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "j"),
        std::make_unique<LiteralExpr>(0.0, 1)
    );
    auto cond = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "j")),
        tok(TokenType::LESS, "<"),
        std::make_unique<LiteralExpr>(3.0, 1)
    );
    auto inc = std::make_unique<AssignExpr>(
        tok(TokenType::IDENTIFIER, "j"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "j")),
            tok(TokenType::PLUS, "+"),
            std::make_unique<LiteralExpr>(1.0, 1)
        )
    );
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "j")), 1
    ));
    stmts.push_back(std::make_unique<ForStmt>(
        std::move(init), std::move(cond), std::move(inc),
        std::make_unique<BlockStmt>(std::move(body))
    ));
    EXPECT_EQ(runStmts(), "0\n1\n2\n");
}

// ── visitBinaryExpr 미구현 연산자 테스트 ────────────────────────────

// print 10 - 3;  →  "7"
TEST_F(ExecutorTest, Subtraction) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(10.0, 1),
            tok(TokenType::MINUS, "-"),
            std::make_unique<LiteralExpr>(3.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "7\n");
}

// print 10 / 4;  →  "2.5"
TEST_F(ExecutorTest, Division) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(10.0, 1),
            tok(TokenType::SLASH, "/"),
            std::make_unique<LiteralExpr>(4.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "2.5\n");
}

// print 5 == 5;  →  "true"
TEST_F(ExecutorTest, EqualEqual_True) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(5.0, 1),
            tok(TokenType::EQUAL_EQUAL, "=="),
            std::make_unique<LiteralExpr>(5.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print 5 == 6;  →  "false"
TEST_F(ExecutorTest, EqualEqual_False) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(5.0, 1),
            tok(TokenType::EQUAL_EQUAL, "=="),
            std::make_unique<LiteralExpr>(6.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "false\n");
}

// print 5 != 6;  →  "true"
TEST_F(ExecutorTest, BangEqual) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(5.0, 1),
            tok(TokenType::BANG_EQUAL, "!="),
            std::make_unique<LiteralExpr>(6.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print 5 > 3;  →  "true"
TEST_F(ExecutorTest, Greater) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(5.0, 1),
            tok(TokenType::GREATER, ">"),
            std::make_unique<LiteralExpr>(3.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print 3 >= 3;  →  "true"
TEST_F(ExecutorTest, GreaterEqual) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(3.0, 1),
            tok(TokenType::GREATER_EQUAL, ">="),
            std::make_unique<LiteralExpr>(3.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print 2 < 5;  →  "true"  (ForLoop와 독립된 단독 검증)
TEST_F(ExecutorTest, Less) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(2.0, 1),
            tok(TokenType::LESS, "<"),
            std::make_unique<LiteralExpr>(5.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print 3 <= 3;  →  "true"
TEST_F(ExecutorTest, LessEqual) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(3.0, 1),
            tok(TokenType::LESS_EQUAL, "<="),
            std::make_unique<LiteralExpr>(3.0, 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "true\n");
}

// print "Hello, " + "World";  →  "Hello, World"
TEST_F(ExecutorTest, StringConcatenation) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(std::string("Hello, "), 1),
            tok(TokenType::PLUS, "+"),
            std::make_unique<LiteralExpr>(std::string("World"), 1)
        ), 1
    ));
    EXPECT_EQ(runStmts(), "Hello, World\n");
}

// ── 오류 케이스 ──────────────────────────────────────────────────────

// 미정의 변수 접근 → 예외
TEST_F(ExecutorTest, UndefinedVariableThrows) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "notDefined")),
        1
    ));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// 3 / 0  → 예외
TEST_F(ExecutorTest, DivisionByZeroThrows) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(3.0, 1),
            tok(TokenType::SLASH, "/"),
            std::make_unique<LiteralExpr>(0.0, 1)
        ), 1
    ));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// true * false  → 예외 (bool은 산술 연산 불가)
TEST_F(ExecutorTest, TypeMismatchThrows) {
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(true, 1),
            tok(TokenType::STAR, "*"),
            std::make_unique<LiteralExpr>(false, 1)
        ), 1
    ));
    EXPECT_THROW(runStmts(), std::runtime_error);
}