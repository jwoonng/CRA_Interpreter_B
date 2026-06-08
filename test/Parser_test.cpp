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

TEST(ParserTest, Modulo) {
    // 10 % 3;
    Parser parser;
    auto stmts = parser.parse({
        num(10), tok(TokenType::PERCENT, "%"), num(3),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::PERCENT);
    auto* lhs = as<LiteralExpr>(bin->left.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lhs->value), 10.0);
    auto* rhs = as<LiteralExpr>(bin->right.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(rhs->value), 3.0);
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

TEST(ParserTest, ModuloBeforeAddition) {
    // 1 + 10 % 3  →  BinaryExpr(1, +, BinaryExpr(10, %, 3))
    // % 가 + 보다 먼저 묶여야 함
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::PLUS, "+"),
        num(10), tok(TokenType::PERCENT, "%"), num(3),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* add = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op.type, TokenType::PLUS);

    auto* mod = as<BinaryExpr>(add->right.get());
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->op.type, TokenType::PERCENT);
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

// ════════════════════════════════════════════════════
// Wave 13 — 연산자 우선순위 심화
// ════════════════════════════════════════════════════

TEST(ParserTest, LogicalAndPrecedenceOverOr) {
    // false or true and false
    // → LogicalExpr(false, or, LogicalExpr(true, and, false))
    // and가 or보다 먼저 묶여야 함
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FALSE_KW, "false", false),
        tok(TokenType::OR,       "or"),
        tok(TokenType::TRUE_KW,  "true",  true),
        tok(TokenType::AND,      "and"),
        tok(TokenType::FALSE_KW, "false", false),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* orExpr = as<LogicalExpr>(es->expression.get());
    ASSERT_NE(orExpr, nullptr);
    EXPECT_EQ(orExpr->op.type, TokenType::OR);

    // or 의 오른쪽이 and 노드여야 함
    auto* andExpr = as<LogicalExpr>(orExpr->right.get());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op.type, TokenType::AND);
}

TEST(ParserTest, ComparisonPrecedenceOverEquality) {
    // 1 < 2 == true
    // → BinaryExpr(BinaryExpr(1, <, 2), ==, true)
    // < 가 == 보다 먼저 묶여야 함
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::LESS, "<"), num(2),
        tok(TokenType::EQUAL_EQUAL, "=="),
        tok(TokenType::TRUE_KW, "true", true),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* eqExpr = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(eqExpr, nullptr);
    EXPECT_EQ(eqExpr->op.type, TokenType::EQUAL_EQUAL);

    // == 의 왼쪽이 < 노드여야 함
    auto* ltExpr = as<BinaryExpr>(eqExpr->left.get());
    ASSERT_NE(ltExpr, nullptr);
    EXPECT_EQ(ltExpr->op.type, TokenType::LESS);
}

// ════════════════════════════════════════════════════
// Wave 14 — 비교 연산자 나머지 3종 (>, >=, <=)
// Note: LessThan(<)은 Wave 5에 있음
// ════════════════════════════════════════════════════

TEST(ParserTest, GreaterThan) {
    // 2 > 1;
    Parser parser;
    auto stmts = parser.parse({
        num(2), tok(TokenType::GREATER, ">"), num(1),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::GREATER);
}

TEST(ParserTest, GreaterEqual) {
    // 2 >= 2;
    Parser parser;
    auto stmts = parser.parse({
        num(2), tok(TokenType::GREATER_EQUAL, ">="), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::GREATER_EQUAL);
}

TEST(ParserTest, LessEqual) {
    // 1 <= 2;
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::LESS_EQUAL, "<="), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* bin = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op.type, TokenType::LESS_EQUAL);
}

// ════════════════════════════════════════════════════
// Wave 15 — 결합성 (Associativity)
// ════════════════════════════════════════════════════

TEST(ParserTest, SubtractionLeftAssociative) {
    // 1 - 2 - 3  →  (1 - 2) - 3
    // BinaryExpr(BinaryExpr(1, -, 2), -, 3)
    Parser parser;
    auto stmts = parser.parse({
        num(1), tok(TokenType::MINUS, "-"),
        num(2), tok(TokenType::MINUS, "-"),
        num(3), tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* outer = as<BinaryExpr>(es->expression.get());
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op.type, TokenType::MINUS);

    // 왼쪽이 또 다른 BinaryExpr(-, ) 이어야 함 (좌결합)
    auto* inner = as<BinaryExpr>(outer->left.get());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op.type, TokenType::MINUS);

    // 가장 오른쪽 피연산자는 3
    auto* rhs = as<LiteralExpr>(outer->right.get());
    ASSERT_NE(rhs, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(rhs->value), 3.0);
}

TEST(ParserTest, AssignmentRightAssociative) {
    // x = y = 1  →  x = (y = 1)
    // AssignExpr(x, AssignExpr(y, 1))
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::IDENTIFIER, "x"),
        tok(TokenType::EQUAL,      "="),
        tok(TokenType::IDENTIFIER, "y"),
        tok(TokenType::EQUAL,      "="),
        num(1),
        tok(TokenType::SEMICOLON, ";"), eof()
    });

    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* outer = as<AssignExpr>(es->expression.get());
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->name.lexeme, "x");

    // 오른쪽이 또 다른 AssignExpr 이어야 함 (우결합)
    auto* inner = as<AssignExpr>(outer->value.get());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name.lexeme, "y");
    auto* val = as<LiteralExpr>(inner->value.get());
    ASSERT_NE(val, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(val->value), 1.0);
}

// ════════════════════════════════════════════════════
// Wave 16 — for 변형 (nullable 절 조합)
// ════════════════════════════════════════════════════

TEST(ParserTest, ForStatementAllClausesEmpty) {
    // for (;;) print 1;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::SEMICOLON,   ";"),   // init 없음
        tok(TokenType::SEMICOLON,   ";"),   // condition 없음
        tok(TokenType::RIGHT_PAREN, ")"),   // increment 없음
        tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    ASSERT_EQ(stmts.size(), 1u);
    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    EXPECT_EQ(fs->initializer, nullptr);
    EXPECT_EQ(fs->condition,   nullptr);
    EXPECT_EQ(fs->increment,   nullptr);
    ASSERT_NE(fs->body,        nullptr);
}

TEST(ParserTest, ForStatementConditionOnly) {
    // for (; i < 3;) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::LESS, "<"), num(3),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT, "print"), tok(TokenType::IDENTIFIER, "i"), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    EXPECT_EQ(fs->initializer, nullptr);
    ASSERT_NE(fs->condition,   nullptr);
    EXPECT_EQ(fs->increment,   nullptr);
}

TEST(ParserTest, ForStatementIncrementOnly) {
    // for (;; i = i + 1) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::SEMICOLON,   ";"),
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
    EXPECT_EQ(fs->condition,   nullptr);
    ASSERT_NE(fs->increment,   nullptr);
}

TEST(ParserTest, ForStatementInitOnly) {
    // for (var i = 0;;) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::EQUAL,      "="), num(0),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT, "print"), tok(TokenType::IDENTIFIER, "i"), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_NE(fs->initializer, nullptr);
    EXPECT_EQ(fs->condition,   nullptr);
    EXPECT_EQ(fs->increment,   nullptr);
}

TEST(ParserTest, ForStatementInitAndCond) {
    // for (var i = 0; i < 3;) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::EQUAL,      "="), num(0),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::LESS, "<"), num(3),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT, "print"), tok(TokenType::IDENTIFIER, "i"), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_NE(fs->initializer, nullptr);
    ASSERT_NE(fs->condition,   nullptr);
    EXPECT_EQ(fs->increment,   nullptr);
}

TEST(ParserTest, ForStatementInitAndIncr) {
    // for (var i = 0;; i = i + 1) print i;
    Parser parser;
    auto stmts = parser.parse({
        tok(TokenType::FOR,         "for"),
        tok(TokenType::LEFT_PAREN,  "("),
        tok(TokenType::VAR,        "var"),
        tok(TokenType::IDENTIFIER, "i"),
        tok(TokenType::EQUAL,      "="), num(0),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::SEMICOLON,   ";"),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::EQUAL, "="),
        tok(TokenType::IDENTIFIER,  "i"), tok(TokenType::PLUS,  "+"), num(1),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::PRINT, "print"), tok(TokenType::IDENTIFIER, "i"), tok(TokenType::SEMICOLON, ";"),
        eof()
    });

    auto* fs = as<ForStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_NE(fs->initializer, nullptr);
    EXPECT_EQ(fs->condition,   nullptr);
    ASSERT_NE(fs->increment,   nullptr);
}

// ════════════════════════════════════════════════════
// Wave 17 — 에러 케이스 추가
// ════════════════════════════════════════════════════

TEST(ParserTest, VarMissingIdentifierThrows) {
    // var ;  — 변수명 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::VAR,       "var"),
            tok(TokenType::SEMICOLON, ";"), eof()
        }),
        std::exception
    );
}

TEST(ParserTest, IfMissingLeftParenThrows) {
    // if true) print 1;  — ( 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::IF,          "if"),
            tok(TokenType::TRUE_KW,     "true", true),
            tok(TokenType::RIGHT_PAREN, ")"),
            tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
            eof()
        }),
        std::exception
    );
}

TEST(ParserTest, IfMissingRightParenThrows) {
    // if (true print 1;  — ) 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::IF,         "if"),
            tok(TokenType::LEFT_PAREN, "("),
            tok(TokenType::TRUE_KW,    "true", true),
            tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
            eof()
        }),
        std::exception
    );
}

TEST(ParserTest, ForMissingLeftParenThrows) {
    // for ;; )  — ( 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::FOR,        "for"),
            tok(TokenType::SEMICOLON,  ";"),
            tok(TokenType::SEMICOLON,  ";"),
            tok(TokenType::RIGHT_PAREN,")"),
            eof()
        }),
        std::exception
    );
}

TEST(ParserTest, ForMissingRightParenThrows) {
    // for ( ;; — ) 없음
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::FOR,        "for"),
            tok(TokenType::LEFT_PAREN, "("),
            tok(TokenType::SEMICOLON,  ";"),
            tok(TokenType::SEMICOLON,  ";"),
            eof()
        }),
        std::exception
    );
}

TEST(ParserTest, UnmatchedClosingBraceThrows) {
    // }  — 열린 스코프 없이 } 단독 사용
    Parser parser;
    EXPECT_THROW(
        parser.parse({
            tok(TokenType::RIGHT_BRACE, "}"),
            eof()
        }),
        std::exception
    );
}

TEST(ParserTest, UnmatchedClosingBraceErrorMessage) {
    // 에러 메시지에 "}" 관련 내용이 포함되어야 함
    Parser parser;
    try {
        parser.parse({
            tok(TokenType::RIGHT_BRACE, "}", {}, 3),
            eof()
        });
        FAIL() << "expected throw";
    } catch (const std::exception& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("}"), std::string::npos) << "message: " << msg;
        EXPECT_NE(msg.find("line 3"), std::string::npos) << "message: " << msg;
    }
}
