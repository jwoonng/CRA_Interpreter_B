#include <gtest/gtest.h>
#include "src/assembler/Parser.h"
#include "src/common/Stmt.h"
#include "src/common/Expr.h"

// ── 개발자 2 담당 영역 ──────────────────────────────
// TDD 순서: Wave 1 → 2 → ... 순서대로 Red-Green-Refactor

// ── 헬퍼 ────────────────────────────────────────────
namespace {

Token tok(TokenType type, std::string lexeme, LiteralValue lit = {}, int line = 1) {
    return Token{ type, std::move(lexeme), std::move(lit), line };
}
Token num(double v, int line = 1)         { return tok(TokenType::NUMBER,     std::to_string(v), v,           line); }
Token str(std::string s, int line = 1)    { return tok(TokenType::STRING,     s,                 s,           line); }
Token eof(int line = 1)                   { return tok(TokenType::EOF_TOKEN,  "",                {},          line); }

// AST 캐스팅 헬퍼
template<typename T>
T* as(Stmt* s) { return dynamic_cast<T*>(s); }
template<typename T>
T* as(Expr* e) { return dynamic_cast<T*>(e); }

} // namespace

// ════════════════════════════════════════════════════
// Wave 1 — 리터럴 (ExpressionStmt + LiteralExpr)
// 목표: parse()가 최소한 리터럴 하나를 ExpressionStmt로 감싸서 반환
// ════════════════════════════════════════════════════

TEST(ParserTest, NumberLiteral) {
    // 42;
    Parser parser;
    auto stmts = parser.parse({
        num(42), tok(TokenType::SEMICOLON, ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* lit = as<LiteralExpr>(es->expression.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lit->value), 42.0);
}

TEST(ParserTest, StringLiteral) {
    // "hello";
    Parser parser;
    auto stmts = parser.parse({
        str("hello"), tok(TokenType::SEMICOLON, ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* lit = as<LiteralExpr>(es->expression.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(std::get<std::string>(lit->value), "hello");
}

TEST(ParserTest, BoolTrueLiteral) {
    // true;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::TRUE_KW, "true", true), tok(TokenType::SEMICOLON, ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* lit = as<LiteralExpr>(es->expression.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_TRUE(std::get<bool>(lit->value));
}

TEST(ParserTest, BoolFalseLiteral) {
    // false;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FALSE_KW, "false", false), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* lit = as<LiteralExpr>(es->expression.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_FALSE(std::get<bool>(lit->value));
}

// ════════════════════════════════════════════════════
// Wave 2 — 단항 연산자 (UnaryExpr)
// ════════════════════════════════════════════════════

TEST(ParserTest, UnaryMinus) {
    // -42;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::MINUS, "-"), num(42), tok(TokenType::SEMICOLON, ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* unary = as<UnaryExpr>(es->expression.get());
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::MINUS);
    auto* operand = as<LiteralExpr>(unary->right.get());
    ASSERT_NE(operand, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(operand->value), 42.0);
}

TEST(ParserTest, UnaryBang) {
    // !true;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::BANG, "!"),
        tok(TokenType::TRUE_KW, "true", true),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* unary = as<UnaryExpr>(es->expression.get());
    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op.type, TokenType::BANG);
}

// ════════════════════════════════════════════════════
// Wave 3 — 사칙연산 (BinaryExpr)
// ════════════════════════════════════════════════════

TEST(ParserTest, Addition) {
    // 1 + 2;
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::PLUS, "+"), num(2), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::PLUS);
    auto* lhs = as<LiteralExpr>(bin->left.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lhs->value), 1.0);
    auto* rhs = as<LiteralExpr>(bin->right.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(rhs->value), 2.0);
}

TEST(ParserTest, Subtraction) {
    // 5 - 3;
    Parser parser;
    auto stmts = parser.parse({
        num(5), tok(TokenType::MINUS, "-"), num(3), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::MINUS);
}

TEST(ParserTest, Multiplication) {
    // 2 * 3;
    Parser parser;
    auto stmts = parser.parse({
        num(2), tok(TokenType::STAR, "*"), num(3), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::STAR);
}

TEST(ParserTest, Division) {
    // 6 / 2;
    Parser parser;
    auto stmts = parser.parse({
        num(6), tok(TokenType::SLASH, "/"), num(2), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::SLASH);
}

// ════════════════════════════════════════════════════
// Wave 4 — 연산자 우선순위
// ════════════════════════════════════════════════════

TEST(ParserTest, MultiplicationBeforeAddition) {
    // 1 + 2 * 3  →  BinaryExpr(1, +, BinaryExpr(2, *, 3))
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::PLUS,  "+"),
        num(2), tok(TokenType::STAR,  "*"), num(3),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* add = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* mul = as<BinaryExpr>(add->right.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);
}

TEST(ParserTest, GroupingOverridesPrecedence) {
    // (1 + 2) * 3  →  BinaryExpr(GroupingExpr(BinaryExpr(1,+,2)), *, 3)
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::LEFT_PAREN, "("),
        num(1), tok(TokenType::PLUS, "+"), num(2),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::STAR, "*"), num(3),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* mul = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op.type, TokenType::STAR);
    ASSERT_NE(as<GroupingExpr>(mul->left.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 5 — 비교 / 논리 연산자
// ════════════════════════════════════════════════════

TEST(ParserTest, EqualEqual) {
    // 1 == 1;
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::EQUAL_EQUAL, "=="), num(1),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::EQUAL_EQUAL);
}

TEST(ParserTest, BangEqual) {
    // 1 != 2;
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::BANG_EQUAL, "!="), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::BANG_EQUAL);
}

TEST(ParserTest, LessThan) {
    // 1 < 2;
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::LESS, "<"), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::LESS);
}

TEST(ParserTest, LogicalAnd) {
    // true and false;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::TRUE_KW,  "true",  true),
        tok(TokenType::AND,      "and"),
        tok(TokenType::FALSE_KW, "false", false),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* logical = as<LogicalExpr>(es->expression.get());
    ASSERT_NE(logical, nullptr);
    EXPECT_EQ(logical->op.type, TokenType::AND);
}

TEST(ParserTest, LogicalOr) {
    // false or true;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FALSE_KW, "false", false),
        tok(TokenType::OR,       "or"),
        tok(TokenType::TRUE_KW,  "true",  true),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* logical = as<LogicalExpr>(es->expression.get());
    ASSERT_NE(logical, nullptr);
    EXPECT_EQ(logical->op.type, TokenType::OR);
}

// ════════════════════════════════════════════════════
// Wave 6 — 변수 선언 / 참조 / 대입 (VarDeclareStmt, VariableExpr, AssignExpr)
// ════════════════════════════════════════════════════

TEST(ParserTest, VarDeclNoInitializer) {
    // var x;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::SEMICOLON,  ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* vd = as<VarDeclareStmt>(stmts[0].get());
    ASSERT_NE(vd, nullptr);
    EXPECT_EQ(vd->name.lexeme, "x");
    EXPECT_EQ(vd->initializer, nullptr);
}

TEST(ParserTest, VarDeclWithInitializer) {
    // var x = 42;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::EQUAL,      "="),
        num(42),
        tok(TokenType::SEMICOLON,  ";"), eof()
    });

    auto* vd = as<VarDeclareStmt>(stmts[0].get());
    ASSERT_NE(vd, nullptr);
    EXPECT_EQ(vd->name.lexeme, "x");
    auto* init = as<LiteralExpr>(vd->initializer.get());
    ASSERT_NE(init, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(init->value), 42.0);
}

TEST(ParserTest, VariableReference) {
    // x;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::SEMICOLON,  ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* var = as<VariableExpr>(es->expression.get());
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name.lexeme, "x");
}

TEST(ParserTest, AssignExpression) {
    // x = 10;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::EQUAL,      "="),
        num(10),
        tok(TokenType::SEMICOLON,  ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* assign = as<AssignExpr>(es->expression.get());
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name.lexeme, "x");
    auto* val = as<LiteralExpr>(assign->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(val->value), 10.0);
}

// ════════════════════════════════════════════════════
// Wave 7 — print 문 (PrintStmt)
// ════════════════════════════════════════════════════

TEST(ParserTest, PrintLiteralStatement) {
    // print 42;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::PRINT,     "print"),
        num(42),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* ps = as<PrintStmt>(stmts[0].get());
    ASSERT_NE(ps, nullptr);
    auto* lit = as<LiteralExpr>(ps->expression.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lit->value), 42.0);
}

TEST(ParserTest, PrintExpressionStatement) {
    // print 1 + 2;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::PRINT,     "print"),
        num(1), tok(TokenType::PLUS, "+"), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* ps = as<PrintStmt>(stmts[0].get());
    ASSERT_NE(ps, nullptr);
    ASSERT_NE(as<BinaryExpr>(ps->expression.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 8 — 블록 (BlockStmt)
// ════════════════════════════════════════════════════

TEST(ParserTest, EmptyBlock) {
    // {}
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::LEFT_BRACE,  "{"),
        tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* block = as<BlockStmt>(stmts[0].get());
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(block->statements.empty());
}

TEST(ParserTest, BlockWithTwoStatements) {
    // { print 1; print 2; }
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::LEFT_BRACE,  "{"),
        tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
        tok(TokenType::PRINT, "print"), num(2), tok(TokenType::SEMICOLON, ";"),
        tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    });

    auto* block = as<BlockStmt>(stmts[0].get());
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->statements.size(), 2u);
}

// ════════════════════════════════════════════════════
// Wave 9 — if / if-else (IfStmt)
// ════════════════════════════════════════════════════

TEST(ParserTest, IfStatement) {
    // if (true) print 1;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::IF,         "if"),
        tok(TokenType::LEFT_PAREN, "("),
        tok(TokenType::TRUE_KW,    "true", true),
        tok(TokenType::RIGHT_PAREN,")"),
        tok(TokenType::PRINT,      "print"), num(1), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* ifs = as<IfStmt>(stmts[0].get());
    ASSERT_NE(ifs, nullptr);
    ASSERT_NE(ifs->thenBranch, nullptr);
    EXPECT_EQ(ifs->elseBranch, nullptr);
}

TEST(ParserTest, IfElseStatement) {
    // if (true) print 1; else print 2;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::IF,          "if"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::TRUE_KW,     "true", true),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT,       "print"), num(1), tok(TokenType::SEMICOLON, ";"),
        tok(TokenType::ELSE,        "else"),
        tok(TokenType::PRINT,       "print"), num(2), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* ifs = as<IfStmt>(stmts[0].get());
    ASSERT_NE(ifs, nullptr);
    ASSERT_NE(ifs->thenBranch,  nullptr);
    ASSERT_NE(ifs->elseBranch,  nullptr);
}

// ════════════════════════════════════════════════════
// Wave 10 — for 문 (ForStmt)
// ════════════════════════════════════════════════════

TEST(ParserTest, ForStatement) {
    // for (var i = 0; i < 3; i = i + 1) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        // init: var i = 0;
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::EQUAL,      "="),
        num(0),
        tok(TokenType::SEMICOLON,  ";"),
        // cond: i < 3
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::LESS,       "<"),
        num(3),
        tok(TokenType::SEMICOLON,  ";"),
        // increment: i = i + 1
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::PLUS,       "+"),
        num(1),
        tok(TokenType::RIGHT_PAREN, ")"),
        // body: print i;
        tok(TokenType::PRINT,      "print"),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_NE(fs->initializer, nullptr);
    ASSERT_NE(fs->condition,   nullptr);
    ASSERT_NE(fs->increment,   nullptr);
    ASSERT_NE(fs->body,        nullptr);
}

TEST(ParserTest, ForStatementNoInit) {
    // for (; i < 3; i = i + 1) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::LESS, "<"), num(3),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::EQUAL, "="),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::PLUS,  "+"), num(1),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT, "print"), tok(TokenType::IDENTIFIER, "i"), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    EXPECT_EQ(fs->initializer, nullptr);
}

// ════════════════════════════════════════════════════
// Wave 11 — 여러 문 연속 파싱
// ════════════════════════════════════════════════════

TEST(ParserTest, MultipleStatements) {
    // var x = 1; print x;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::EQUAL,      "="),
        num(1),
        tok(TokenType::SEMICOLON,  ";"),
        tok(TokenType::PRINT,      "print"),
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::SEMICOLON,  ";"),
        eof()
    });

    ASSERT_EQ(stmts.size(), 2u);
    ASSERT_NE(as<VarDeclareStmt>(stmts[0].get()), nullptr);
    ASSERT_NE(as<PrintStmt>(stmts[1].get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 12 — 에러 케이스 (예외 throw 확인)
// ════════════════════════════════════════════════════

TEST(ParserTest, MissingSemicolonThrows) {
    // 42  (세미콜론 없음)
    Parser parser;
    EXPECT_THROW(parser.parse({ num(42), eof() }), std::exception);
}

TEST(ParserTest, UnclosedParenThrows) {
    // (1 + 2;  — 닫힌 괄호 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::LEFT_PAREN, "("),
            num(1), tok(TokenType::PLUS, "+"), num(2),
            tok(TokenType::SEMICOLON, ";"), eof()
        }),
        std::exception
    );
}

TEST(ParserTest, UnclosedBraceThrows) {
    // { print 1;  — 닫힌 중괄호 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::LEFT_BRACE, "{"),
            tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
            eof()
        }),
        std::exception
    );
}
