#include <gtest/gtest.h>
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/executor/Executor.h"
#include "src/checker/Checker.h"
#include "src/common/Stmt.h"
#include "src/common/Expr.h"
#include <sstream>

// ── helpers ──────────────────────────────────────────────────────────
namespace {

Token tok(TokenType type, std::string lexeme, LiteralValue lit = {}, int line = 1) {
    return Token{ type, std::move(lexeme), std::move(lit), line };
}
Token num(double v, int line = 1) {
    return tok(TokenType::NUMBER, std::to_string(v), v, line);
}
Token id(std::string s, int line = 1) {
    return tok(TokenType::IDENTIFIER, std::move(s), {}, line);
}
Token eof(int line = 1) { return tok(TokenType::EOF_TOKEN, "", {}, line); }

template<typename T> T* as(Stmt* s) { return dynamic_cast<T*>(s); }
template<typename T> T* as(Expr* e) { return dynamic_cast<T*>(e); }

ExprPtr varExpr(const std::string& name, int line = 1) {
    return std::make_unique<VariableExpr>(id(name, line));
}
ExprPtr litNum(double v, int line = 1) {
    return std::make_unique<LiteralExpr>(LiteralValue{v}, line);
}
ExprPtr litStr(std::string s, int line = 1) {
    return std::make_unique<LiteralExpr>(LiteralValue{std::move(s)}, line);
}
ExprPtr binExpr(ExprPtr l, TokenType op, std::string opLex, ExprPtr r, int line = 1) {
    return std::make_unique<BinaryExpr>(
        std::move(l), tok(op, std::move(opLex), {}, line), std::move(r));
}

StmtPtr printStmt(ExprPtr e, int line = 1) {
    return std::make_unique<PrintStmt>(std::move(e), line);
}
StmtPtr varDecl(const std::string& name, ExprPtr init = nullptr, int line = 1) {
    return std::make_unique<VarDeclareStmt>(id(name, line), std::move(init));
}
StmtPtr exprStmt(ExprPtr e) {
    return std::make_unique<ExpressionStmt>(std::move(e));
}

// Array(n) 호출 식 생성
ExprPtr makeArrayCall(LiteralValue sizeLit, int line = 1) {
    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(std::move(sizeLit), line));
    return std::make_unique<CallExpr>(
        varExpr("Array", line),
        tok(TokenType::RIGHT_PAREN, ")", {}, line),
        std::move(args));
}
ExprPtr makeArrayCall(double n, int line = 1) {
    return makeArrayCall(LiteralValue{n}, line);
}

// unique_ptr<IndexExpr> 반환 — IndexAssignExpr 생성에 사용 (타입 안전)
std::unique_ptr<IndexExpr> makeIndexExpr(ExprPtr obj, ExprPtr idx, int line = 1) {
    return std::make_unique<IndexExpr>(
        std::move(obj), std::move(idx),
        tok(TokenType::RIGHT_BRACKET, "]", {}, line));
}

// ExprPtr 반환 — 읽기 컨텍스트(printStmt 등)에 사용
ExprPtr makeIndex(ExprPtr obj, ExprPtr idx, int line = 1) {
    return makeIndexExpr(std::move(obj), std::move(idx), line);
}

// arr[idx] = val — unique_ptr<IndexExpr>로 타입 레벨에서 IndexExpr 강제
ExprPtr makeIndexAssign(std::unique_ptr<IndexExpr> target, ExprPtr val, int line = 1) {
    return std::make_unique<IndexAssignExpr>(std::move(target), std::move(val), line);
}

} // namespace

// ── Executor fixture ─────────────────────────────────────────────────
class ArrayExecutorTest : public ::testing::Test {
protected:
    Executor ex;
    std::ostringstream oss;
    std::vector<StmtPtr> stmts;

    std::string runStmts() {
        ex.execute(std::move(stmts), oss);
        return oss.str();
    }
};

// ── Checker fixture ───────────────────────────────────────────────────
class ArrayCheckerTest : public ::testing::Test {
protected:
    Checker checker;
    std::vector<StmtPtr> stmts;
};

// ════════════════════════════════════════════════════
// Wave 1 — Tokenizer: [, ]
// ════════════════════════════════════════════════════

TEST(ArrayTokenizerTest, LeftBracket) {
    Tokenizer t;
    auto tokens = t.tokenize("[");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::LEFT_BRACKET);
    EXPECT_EQ(tokens[0].lexeme, "[");
}

TEST(ArrayTokenizerTest, RightBracket) {
    Tokenizer t;
    auto tokens = t.tokenize("]");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::RIGHT_BRACKET);
    EXPECT_EQ(tokens[0].lexeme, "]");
}

TEST(ArrayTokenizerTest, IndexAccess_TokenStream) {
    // arr[0]  →  IDENTIFIER [ NUMBER ] EOF
    Tokenizer t;
    auto tokens = t.tokenize("arr[0]");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::LEFT_BRACKET);
    EXPECT_EQ(tokens[2].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[3].type, TokenType::RIGHT_BRACKET);
    EXPECT_EQ(tokens[4].type, TokenType::EOF_TOKEN);
}

TEST(ArrayTokenizerTest, IndexAssign_FullTokenStream) {
    // arr[0] = 10;  →  IDENTIFIER [ NUMBER ] = NUMBER ;
    Tokenizer t;
    auto tokens = t.tokenize("arr[0] = 10;");
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::LEFT_BRACKET);
    EXPECT_EQ(tokens[2].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[3].type, TokenType::RIGHT_BRACKET);
    EXPECT_EQ(tokens[4].type, TokenType::EQUAL);
    EXPECT_EQ(tokens[5].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[6].type, TokenType::SEMICOLON);
}

// ════════════════════════════════════════════════════
// Wave 2 — Parser: Index read
// ════════════════════════════════════════════════════

TEST(ArrayParserTest, IndexRead_SimpleInteger) {
    // arr[0];
    Parser p;
    auto stmts = p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["), num(0), tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* idx = as<IndexExpr>(es->expression.get());
    ASSERT_NE(idx, nullptr);
    auto* obj = as<VariableExpr>(idx->object.get());
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->name.lexeme, "arr");
    auto* lit = as<LiteralExpr>(idx->index.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lit->value), 0.0);
}

TEST(ArrayParserTest, IndexRead_ExpressionIndex) {
    // arr[i + 1];
    Parser p;
    auto stmts = p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["),
        id("i"), tok(TokenType::PLUS, "+"), num(1),
        tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* idx = as<IndexExpr>(es->expression.get());
    ASSERT_NE(idx, nullptr);
    EXPECT_NE(as<BinaryExpr>(idx->index.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 3 — Parser: Array() creation call
// ════════════════════════════════════════════════════

TEST(ArrayParserTest, ArrayCall_ExprStatement) {
    // Array(3);
    Parser p;
    auto stmts = p.parse({
        id("Array"),
        tok(TokenType::LEFT_PAREN, "("), num(3), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* call = as<CallExpr>(es->expression.get());
    ASSERT_NE(call, nullptr);
    ASSERT_EQ(call->arguments.size(), 1u);
    auto* callee = as<VariableExpr>(call->callee.get());
    ASSERT_NE(callee, nullptr);
    EXPECT_EQ(callee->name.lexeme, "Array");
}

TEST(ArrayParserTest, ArrayCall_VarDecl) {
    // var arr = Array(5);
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::VAR, "var"), id("arr"), tok(TokenType::EQUAL, "="),
        id("Array"),
        tok(TokenType::LEFT_PAREN, "("), num(5), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* vd = as<VarDeclareStmt>(stmts[0].get());
    ASSERT_NE(vd, nullptr);
    EXPECT_EQ(vd->name.lexeme, "arr");
    EXPECT_NE(as<CallExpr>(vd->initializer.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 4 — Parser: Index assignment
// ════════════════════════════════════════════════════

TEST(ArrayParserTest, IndexAssign_Simple) {
    // arr[0] = 10;
    Parser p;
    auto stmts = p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["), num(0), tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::EQUAL, "="), num(10),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* ia = as<IndexAssignExpr>(es->expression.get());
    ASSERT_NE(ia, nullptr);
    EXPECT_NE(as<IndexExpr>(ia->target.get()), nullptr);
    EXPECT_NE(as<LiteralExpr>(ia->value.get()), nullptr);
}

TEST(ArrayParserTest, IndexAssign_ExpressionIndex) {
    // arr[i - 1] = 7;
    Parser p;
    auto stmts = p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["),
        id("i"), tok(TokenType::MINUS, "-"), num(1),
        tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::EQUAL, "="), num(7),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    EXPECT_NE(as<IndexAssignExpr>(es->expression.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 5 — Parser: Error cases
// ════════════════════════════════════════════════════

TEST(ArrayParserTest, IndexMissingExpression_Throws) {
    // arr[];  →  Expected expression
    Parser p;
    EXPECT_THROW(p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["), tok(TokenType::RIGHT_BRACKET, "]"),
        tok(TokenType::SEMICOLON, ";"), eof()
    }), std::exception);
}

TEST(ArrayParserTest, IndexMissingRightBracket_Throws) {
    // arr[0;  →  Expected ']'
    Parser p;
    EXPECT_THROW(p.parse({
        id("arr"),
        tok(TokenType::LEFT_BRACKET, "["), num(0),
        tok(TokenType::SEMICOLON, ";"), eof()
    }), std::exception);
}

// ════════════════════════════════════════════════════
// Wave 6 — Executor: Array creation
// ════════════════════════════════════════════════════

// var arr = Array(3);  →  no crash
TEST_F(ArrayExecutorTest, ArrayCreate_SizeThree_NoThrow) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    EXPECT_NO_THROW(runStmts());
}

// var arr = Array(0);  →  empty array, no crash
TEST_F(ArrayExecutorTest, ArrayCreate_SizeZero_NoThrow) {
    stmts.push_back(varDecl("arr", makeArrayCall(0)));
    EXPECT_NO_THROW(runStmts());
}

// 초기 요소는 모두 nil
TEST_F(ArrayExecutorTest, ArrayCreate_InitialElementsAreNil) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(0))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(1))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(2))));
    EXPECT_EQ(runStmts(), "nil\nnil\nnil\n");
}

// ════════════════════════════════════════════════════
// Wave 7 — Executor: Read / Write
// ════════════════════════════════════════════════════

// arr[0] = 10;  print arr[0];  →  "10\n"
TEST_F(ArrayExecutorTest, IndexWrite_ThenRead) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litNum(10))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(0))));
    EXPECT_EQ(runStmts(), "10\n");
}

// 여러 요소 읽기/쓰기
TEST_F(ArrayExecutorTest, IndexWrite_MultipleElements) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litNum(10))));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(1)), litNum(20))));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(2)), litNum(30))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(0))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(1))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(2))));
    EXPECT_EQ(runStmts(), "10\n20\n30\n");
}

// 동적 인덱스: var i = 2; arr[i - 1] = 7; print arr[1];  →  "7\n"
TEST_F(ArrayExecutorTest, IndexWrite_DynamicIndex) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(varDecl("i", litNum(2)));
    stmts.push_back(exprStmt(makeIndexAssign(
        makeIndexExpr(varExpr("arr"), binExpr(varExpr("i"), TokenType::MINUS, "-", litNum(1))),
        litNum(7))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(1))));
    EXPECT_EQ(runStmts(), "7\n");
}

// IndexAssign 식의 평가값 = 대입된 값
TEST_F(ArrayExecutorTest, IndexAssign_ReturnsAssignedValue) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(varDecl("v",
        makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litNum(42))));
    stmts.push_back(printStmt(varExpr("v")));
    EXPECT_EQ(runStmts(), "42\n");
}

// 문자열 값 저장/읽기
TEST_F(ArrayExecutorTest, IndexWrite_StringValue) {
    stmts.push_back(varDecl("arr", makeArrayCall(2)));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litStr("hello"))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(0))));
    EXPECT_EQ(runStmts(), "hello\n");
}

// 배열은 참조 타입: 같은 배열을 두 변수가 공유
TEST_F(ArrayExecutorTest, Array_ReferenceSemantics) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(varDecl("brr", varExpr("arr")));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litNum(99))));
    stmts.push_back(printStmt(makeIndex(varExpr("brr"), litNum(0))));
    EXPECT_EQ(runStmts(), "99\n");
}

// ════════════════════════════════════════════════════
// Wave 8 — Executor: Error cases
// ════════════════════════════════════════════════════

// arr[5] on Array(3)  →  out of range
TEST_F(ArrayExecutorTest, IndexRead_OutOfRange_Throws) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(5))));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// arr[-1]  →  out of range
TEST_F(ArrayExecutorTest, IndexRead_NegativeIndex_Throws) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(-1))));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// arr["hello"]  →  index must be integer
TEST_F(ArrayExecutorTest, IndexRead_StringIndex_Throws) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litStr("hello"))));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// var x = 10; print x[0];  →  not an array
TEST_F(ArrayExecutorTest, IndexRead_NonArray_Throws) {
    stmts.push_back(varDecl("x", litNum(10)));
    stmts.push_back(printStmt(makeIndex(varExpr("x"), litNum(0))));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Array("hi")  →  size must be non-negative integer
TEST_F(ArrayExecutorTest, ArrayCreate_StringSize_Throws) {
    stmts.push_back(varDecl("arr", makeArrayCall(LiteralValue{std::string("hi")})));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Array(3.5)  →  size must be non-negative integer (소수 불허)
TEST_F(ArrayExecutorTest, ArrayCreate_FloatSize_Throws) {
    stmts.push_back(varDecl("arr", makeArrayCall(LiteralValue{3.5})));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Array()  →  argument count error
TEST_F(ArrayExecutorTest, ArrayCreate_NoArgs_Throws) {
    std::vector<ExprPtr> noArgs;
    stmts.push_back(exprStmt(std::make_unique<CallExpr>(
        varExpr("Array"),
        tok(TokenType::RIGHT_PAREN, ")"),
        std::move(noArgs))));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// ════════════════════════════════════════════════════
// Wave 9 — Checker: AST 순회 (정적 오류 없음)
// ════════════════════════════════════════════════════

// 정상 배열 사용 → no throw
TEST_F(ArrayCheckerTest, ValidArrayUsage_NoThrow) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(exprStmt(makeIndexAssign(makeIndexExpr(varExpr("arr"), litNum(0)), litNum(10))));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), litNum(0))));
    EXPECT_NO_THROW(checker.check(stmts));
}

// 인덱스에 변수 식 → no throw
TEST_F(ArrayCheckerTest, IndexWithVariable_NoThrow) {
    stmts.push_back(varDecl("arr", makeArrayCall(3)));
    stmts.push_back(varDecl("i", litNum(0)));
    stmts.push_back(printStmt(makeIndex(varExpr("arr"), varExpr("i"))));
    EXPECT_NO_THROW(checker.check(stmts));
}
