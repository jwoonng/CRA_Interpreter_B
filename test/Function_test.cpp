#include <gtest/gtest.h>
#include "src/assembler/Parser.h"
#include "src/executor/Executor.h"
#include "src/checker/Checker.h"
#include "src/common/Stmt.h"
#include "src/common/Expr.h"
#include <sstream>

// ── helpers ──────────────────────────────────────────────────────────
namespace {

// Token factories
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

// Expr builders
ExprPtr varExpr(const std::string& name, int line = 1) {
    return std::make_unique<VariableExpr>(id(name, line));
}
ExprPtr litNum(double v, int line = 1) {
    return std::make_unique<LiteralExpr>(v, line);
}
ExprPtr litStr(std::string s, int line = 1) {
    return std::make_unique<LiteralExpr>(std::move(s), line);
}
ExprPtr binExpr(ExprPtr l, TokenType op, std::string opLex, ExprPtr r, int line = 1) {
    return std::make_unique<BinaryExpr>(
        std::move(l), tok(op, std::move(opLex), {}, line), std::move(r));
}

// Stmt builders
StmtPtr retStmt(ExprPtr val = nullptr, int line = 1) {
    return std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return", {}, line), std::move(val));
}
StmtPtr printStmt(ExprPtr e, int line = 1) {
    return std::make_unique<PrintStmt>(std::move(e), line);
}
StmtPtr varDecl(const std::string& name, ExprPtr init = nullptr, int line = 1) {
    return std::make_unique<VarDeclareStmt>(id(name, line), std::move(init));
}

// Function / call builders
StmtPtr makeFuncDecl(
    const std::string& name,
    std::vector<std::string> paramNames,
    std::vector<StmtPtr> body,
    int line = 1)
{
    std::vector<Token> params;
    for (auto& p : paramNames)
        params.push_back(id(p, line));
    return std::make_unique<FunctionStmt>(
        id(name, line), std::move(params), std::move(body));
}

ExprPtr makeCallExpr(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    return std::make_unique<CallExpr>(
        varExpr(funcName, line),
        tok(TokenType::RIGHT_PAREN, ")", {}, line),
        std::move(args));
}

StmtPtr makeCallStmt(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    return std::make_unique<ExpressionStmt>(
        makeCallExpr(funcName, std::move(args), line));
}

} // namespace

// ── Executor fixture ─────────────────────────────────────────────────
class FunctionExecutorTest : public ::testing::Test {
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
class FunctionCheckerTest : public ::testing::Test {
protected:
    Checker checker;
    std::vector<StmtPtr> stmts;
};

// ════════════════════════════════════════════════════
// Wave 1 — Parser: Function Declaration
// ════════════════════════════════════════════════════

TEST(FunctionParserTest, FuncDecl_NoParams) {
    // Func foo() {}
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::FUN, "func"), id("foo"),
        tok(TokenType::LEFT_PAREN,  "("), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE,  "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* fs = as<FunctionStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    EXPECT_EQ(fs->name.lexeme, "foo");
    EXPECT_TRUE(fs->params.empty());
    EXPECT_TRUE(fs->body.empty());
}

TEST(FunctionParserTest, FuncDecl_WithTwoParams) {
    // Func add(a, b) {}
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::FUN, "func"), id("add"),
        tok(TokenType::LEFT_PAREN,  "("),
        id("a"), tok(TokenType::COMMA, ","), id("b"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE,  "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    });
    auto* fs = as<FunctionStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(fs->params.size(), 2u);
    EXPECT_EQ(fs->params[0].lexeme, "a");
    EXPECT_EQ(fs->params[1].lexeme, "b");
}

TEST(FunctionParserTest, FuncDecl_WithBody) {
    // Func foo() { print 1; }
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::FUN, "func"), id("foo"),
        tok(TokenType::LEFT_PAREN, "("), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE, "{"),
            tok(TokenType::PRINT, "print"), num(1), tok(TokenType::SEMICOLON, ";"),
        tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    });
    auto* fs = as<FunctionStmt>(stmts[0].get());
    ASSERT_NE(fs, nullptr);
    ASSERT_EQ(fs->body.size(), 1u);
    EXPECT_NE(as<PrintStmt>(fs->body[0].get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 2 — Parser: Return Statement
// ════════════════════════════════════════════════════

TEST(FunctionParserTest, ReturnStmt_WithValue) {
    // return 42;
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::RETURN, "return"), num(42), tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* rs = as<ReturnStmt>(stmts[0].get());
    ASSERT_NE(rs, nullptr);
    ASSERT_NE(rs->value, nullptr);
    auto* lit = as<LiteralExpr>(rs->value.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(lit->value), 42.0);
}

TEST(FunctionParserTest, ReturnStmt_WithoutValue) {
    // return;
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::RETURN, "return"), tok(TokenType::SEMICOLON, ";"), eof()
    });
    auto* rs = as<ReturnStmt>(stmts[0].get());
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(rs->value, nullptr);
}

// ════════════════════════════════════════════════════
// Wave 3 — Parser: Call Expression
// ════════════════════════════════════════════════════

TEST(FunctionParserTest, CallExpr_NoArgs) {
    // foo();
    Parser p;
    auto stmts = p.parse({
        id("foo"),
        tok(TokenType::LEFT_PAREN, "("), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* call = as<CallExpr>(es->expression.get());
    ASSERT_NE(call, nullptr);
    EXPECT_TRUE(call->arguments.empty());
    auto* callee = as<VariableExpr>(call->callee.get());
    ASSERT_NE(callee, nullptr);
    EXPECT_EQ(callee->name.lexeme, "foo");
}

TEST(FunctionParserTest, CallExpr_WithTwoArgs) {
    // add(1, 2);
    Parser p;
    auto stmts = p.parse({
        id("add"),
        tok(TokenType::LEFT_PAREN, "("),
        num(1), tok(TokenType::COMMA, ","), num(2),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    auto* es = as<ExpressionStmt>(stmts[0].get());
    ASSERT_NE(es, nullptr);
    auto* call = as<CallExpr>(es->expression.get());
    ASSERT_NE(call, nullptr);
    ASSERT_EQ(call->arguments.size(), 2u);
}

TEST(FunctionParserTest, CallExpr_ResultAssigned) {
    // var r = add(3, 7);
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::VAR, "var"), id("r"), tok(TokenType::EQUAL, "="),
        id("add"),
        tok(TokenType::LEFT_PAREN, "("),
        num(3), tok(TokenType::COMMA, ","), num(7),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::SEMICOLON, ";"), eof()
    });
    ASSERT_EQ(stmts.size(), 1u);
    auto* vd = as<VarDeclareStmt>(stmts[0].get());
    ASSERT_NE(vd, nullptr);
    EXPECT_NE(as<CallExpr>(vd->initializer.get()), nullptr);
}

// ════════════════════════════════════════════════════
// Wave 4 — Parser: Error Cases
// ════════════════════════════════════════════════════

TEST(FunctionParserTest, FuncMissingName_Throws) {
    // Func () {}
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "func"),
        tok(TokenType::LEFT_PAREN, "("), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingLeftParen_Throws) {
    // Func foo) {}
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "func"), id("foo"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingRightParen_Throws) {
    // Func foo( {}
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "func"), id("foo"),
        tok(TokenType::LEFT_PAREN, "("),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingLeftBrace_Throws) {
    // Func foo()  (no brace)
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "func"), id("foo"),
        tok(TokenType::LEFT_PAREN, "("), tok(TokenType::RIGHT_PAREN, ")"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, CallMissingRightParen_Throws) {
    // add(1, 2;
    Parser p;
    EXPECT_THROW(p.parse({
        id("add"),
        tok(TokenType::LEFT_PAREN, "("),
        num(1), tok(TokenType::COMMA, ","), num(2),
        tok(TokenType::SEMICOLON, ";"), eof()
    }), std::exception);
}

// ════════════════════════════════════════════════════
// Wave 5 — Executor: Basic Function Execution
// ════════════════════════════════════════════════════

// Func greet() { print "hello"; }   greet();   -> "hello\n"
TEST_F(FunctionExecutorTest, FuncCall_PrintsFromBody) {
    std::vector<StmtPtr> body;
    body.push_back(printStmt(litStr("hello")));
    stmts.push_back(makeFuncDecl("greet", {}, std::move(body)));
    stmts.push_back(makeCallStmt("greet", {}));
    EXPECT_EQ(runStmts(), "hello\n");
}

// Func add(a, b) { return a + b; }   var r = add(3, 7);   print r;   -> "10\n"
TEST_F(FunctionExecutorTest, FuncCall_ReturnsValue) {
    std::vector<StmtPtr> body;
    body.push_back(retStmt(binExpr(varExpr("a"), TokenType::PLUS, "+", varExpr("b"))));
    stmts.push_back(makeFuncDecl("add", {"a", "b"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(litNum(3.0));
    args.push_back(litNum(7.0));
    stmts.push_back(varDecl("r", makeCallExpr("add", std::move(args))));
    stmts.push_back(printStmt(varExpr("r")));
    EXPECT_EQ(runStmts(), "10\n");
}

// Func noop() {}   var r = noop();   print r;   -> "nil\n"
TEST_F(FunctionExecutorTest, FuncCall_ReturnsNilWhenNoReturn) {
    stmts.push_back(makeFuncDecl("noop", {}, {}));
    stmts.push_back(varDecl("r", makeCallExpr("noop", {})));
    stmts.push_back(printStmt(varExpr("r")));
    EXPECT_EQ(runStmts(), "nil\n");
}

// Func ret_nil() { return; }   var r = ret_nil();   print r;   -> "nil\n"
TEST_F(FunctionExecutorTest, FuncCall_ExplicitReturnNil) {
    std::vector<StmtPtr> body;
    body.push_back(retStmt());
    stmts.push_back(makeFuncDecl("ret_nil", {}, std::move(body)));
    stmts.push_back(varDecl("r", makeCallExpr("ret_nil", {})));
    stmts.push_back(printStmt(varExpr("r")));
    EXPECT_EQ(runStmts(), "nil\n");
}

// Func dbl(x) { return x * 2; }   print dbl(5);   -> "10\n"
TEST_F(FunctionExecutorTest, FuncCall_WithParameters) {
    std::vector<StmtPtr> body;
    body.push_back(retStmt(binExpr(varExpr("x"), TokenType::STAR, "*", litNum(2.0))));
    stmts.push_back(makeFuncDecl("dbl", {"x"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(litNum(5.0));
    stmts.push_back(printStmt(makeCallExpr("dbl", std::move(args))));
    EXPECT_EQ(runStmts(), "10\n");
}

// Global var a=999 must not be modified by function param a
TEST_F(FunctionExecutorTest, FuncCall_ParamsAreLocal) {
    std::vector<StmtPtr> body;
    body.push_back(retStmt(binExpr(varExpr("a"), TokenType::PLUS, "+", varExpr("b"))));
    stmts.push_back(makeFuncDecl("add", {"a", "b"}, std::move(body)));
    stmts.push_back(varDecl("a", litNum(999.0)));

    std::vector<ExprPtr> args;
    args.push_back(litNum(3.0));
    args.push_back(litNum(7.0));
    stmts.push_back(makeCallStmt("add", std::move(args)));
    stmts.push_back(printStmt(varExpr("a")));
    EXPECT_EQ(runStmts(), "999\n");
}

// ════════════════════════════════════════════════════
// Wave 6 — Executor: Recursion
// ════════════════════════════════════════════════════

// Func fact(n) { if (n <= 1) return 1; return n * fact(n-1); }
// print fact(5);  -> "120\n"
TEST_F(FunctionExecutorTest, FuncCall_Recursive_Factorial) {
    std::vector<ExprPtr> recArgs;
    recArgs.push_back(binExpr(varExpr("n"), TokenType::MINUS, "-", litNum(1.0)));

    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<IfStmt>(
        binExpr(varExpr("n"), TokenType::LESS_EQUAL, "<=", litNum(1.0)),
        retStmt(litNum(1.0))));
    body.push_back(retStmt(
        binExpr(varExpr("n"), TokenType::STAR, "*",
            makeCallExpr("fact", std::move(recArgs)))));
    stmts.push_back(makeFuncDecl("fact", {"n"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(litNum(5.0));
    stmts.push_back(printStmt(makeCallExpr("fact", std::move(args))));
    EXPECT_EQ(runStmts(), "120\n");
}

// ════════════════════════════════════════════════════
// Wave 7 — Executor: Error Cases
// ════════════════════════════════════════════════════

// Func foo(a) {}   foo(1, 2);   -> throws (too many args)
TEST_F(FunctionExecutorTest, FuncCall_TooManyArgs_Throws) {
    stmts.push_back(makeFuncDecl("foo", {"a"}, {}));
    std::vector<ExprPtr> args;
    args.push_back(litNum(1.0));
    args.push_back(litNum(2.0));
    stmts.push_back(makeCallStmt("foo", std::move(args)));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Func foo(a, b) {}   foo(1);   -> throws (too few args)
TEST_F(FunctionExecutorTest, FuncCall_TooFewArgs_Throws) {
    stmts.push_back(makeFuncDecl("foo", {"a", "b"}, {}));
    std::vector<ExprPtr> args;
    args.push_back(litNum(1.0));
    stmts.push_back(makeCallStmt("foo", std::move(args)));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// var x = "hello";   x();   -> throws (not a function)
TEST_F(FunctionExecutorTest, FuncCall_NonCallableVariable_Throws) {
    stmts.push_back(varDecl("x", litStr("hello")));
    stmts.push_back(makeCallStmt("x", {}));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// undefinedFunc();   -> throws
TEST_F(FunctionExecutorTest, FuncCall_UndefinedFunction_Throws) {
    stmts.push_back(makeCallStmt("undefinedFunc", {}));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Func add(a) { return a+1; }  Func add(a) { return a+10; }  print add(5);  -> "15\n"
TEST_F(FunctionExecutorTest, FuncDecl_DuplicateName_LastDefinitionWins) {
    {
        std::vector<StmtPtr> body;
        body.push_back(retStmt(binExpr(varExpr("a"), TokenType::PLUS, "+", litNum(1.0))));
        stmts.push_back(makeFuncDecl("add", {"a"}, std::move(body)));
    }
    {
        std::vector<StmtPtr> body;
        body.push_back(retStmt(binExpr(varExpr("a"), TokenType::PLUS, "+", litNum(10.0))));
        stmts.push_back(makeFuncDecl("add", {"a"}, std::move(body)));
    }
    std::vector<ExprPtr> args;
    args.push_back(litNum(5.0));
    stmts.push_back(printStmt(makeCallExpr("add", std::move(args))));
    EXPECT_EQ(runStmts(), "15\n");
}

// ════════════════════════════════════════════════════
// Wave 8 — Checker: Valid Cases
// ════════════════════════════════════════════════════

// Func foo() {}  -> no throw
TEST_F(FunctionCheckerTest, FuncDecl_NoThrow) {
    stmts.push_back(makeFuncDecl("foo", {}, {}));
    EXPECT_NO_THROW(checker.check(stmts));
}

// Func foo() { return 1; }  -> no throw
TEST_F(FunctionCheckerTest, ReturnInsideFunction_NoThrow) {
    std::vector<StmtPtr> body;
    body.push_back(retStmt(litNum(1.0)));
    stmts.push_back(makeFuncDecl("foo", {}, std::move(body)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// Func foo() { { return 1; } }  -> no throw (nested block inside function)
TEST_F(FunctionCheckerTest, ReturnInsideNestedBlock_NoThrow) {
    std::vector<StmtPtr> inner;
    inner.push_back(retStmt(litNum(1.0)));
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    stmts.push_back(makeFuncDecl("foo", {}, std::move(body)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 9 — Checker: Error Cases
// ════════════════════════════════════════════════════

// return 5;  (global scope)  -> throws CheckError
TEST_F(FunctionCheckerTest, ReturnOutsideFunction_Throws) {
    stmts.push_back(retStmt(litNum(5.0)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// return; inside block but outside function  -> throws CheckError
TEST_F(FunctionCheckerTest, ReturnInsideBlock_OutsideFunction_Throws) {
    std::vector<StmtPtr> inner;
    inner.push_back(retStmt());
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// Func foo(a, a) {}  -> throws CheckError
TEST_F(FunctionCheckerTest, DuplicateParams_Throws) {
    std::vector<Token> params = {
        tok(TokenType::IDENTIFIER, "a", {}, 1),
        tok(TokenType::IDENTIFIER, "a", {}, 1)
    };
    stmts.push_back(std::make_unique<FunctionStmt>(
        id("foo"), std::move(params), std::vector<StmtPtr>{}));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// Duplicate param error message contains the param name
TEST_F(FunctionCheckerTest, DuplicateParams_ErrorContainsName) {
    std::vector<Token> params = {
        tok(TokenType::IDENTIFIER, "x", {}, 1),
        tok(TokenType::IDENTIFIER, "x", {}, 2)
    };
    stmts.push_back(std::make_unique<FunctionStmt>(
        id("foo"), std::move(params), std::vector<StmtPtr>{}));
    try {
        checker.check(stmts);
        FAIL() << "Expected CheckError";
    } catch (const CheckError& e) {
        EXPECT_NE(std::string(e.what()).find("x"), std::string::npos);
    }
}
