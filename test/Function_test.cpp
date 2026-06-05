#include <gtest/gtest.h>
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

} // namespace

// ── AST builder helpers (used in Executor tests) ─────────────────────
static StmtPtr makeFuncDecl(
    const std::string& name,
    std::vector<std::string> paramNames,
    std::vector<StmtPtr> body,
    int line = 1)
{
    std::vector<Token> params;
    for (auto& p : paramNames)
        params.push_back(Token{ TokenType::IDENTIFIER, p, {}, line });
    return std::make_unique<FunctionStmt>(
        Token{ TokenType::IDENTIFIER, name, {}, line },
        std::move(params),
        std::move(body));
}

static ExprPtr makeCallExpr(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    auto callee = std::make_unique<VariableExpr>(
        Token{ TokenType::IDENTIFIER, funcName, {}, line });
    Token paren{ TokenType::RIGHT_PAREN, ")", {}, line };
    return std::make_unique<CallExpr>(
        std::move(callee), paren, std::move(args));
}

static StmtPtr makeCallStmt(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    return std::make_unique<ExpressionStmt>(
        makeCallExpr(funcName, std::move(args), line));
}

// ── Executor fixture ─────────────────────────────────────────────────
class FunctionExecutorTest : public ::testing::Test {
protected:
    Executor ex;
    std::ostringstream oss;
    std::vector<StmtPtr> stmts;

    std::string runStmts() {
        ex.execute(stmts, oss);
        return oss.str();
    }
};

// ════════════════════════════════════════════════════
// Wave 1 — Parser: Function Declaration
// ════════════════════════════════════════════════════

TEST(FunctionParserTest, FuncDecl_NoParams) {
    // Func foo() {}
    Parser p;
    auto stmts = p.parse({
        tok(TokenType::FUN, "Func"), id("foo"),
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
        tok(TokenType::FUN, "Func"), id("add"),
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
        tok(TokenType::FUN, "Func"), id("foo"),
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
        tok(TokenType::FUN, "Func"),
        tok(TokenType::LEFT_PAREN, "("), tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingLeftParen_Throws) {
    // Func foo) {}
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "Func"), id("foo"),
        tok(TokenType::RIGHT_PAREN, ")"),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingRightParen_Throws) {
    // Func foo( {}
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "Func"), id("foo"),
        tok(TokenType::LEFT_PAREN, "("),
        tok(TokenType::LEFT_BRACE, "{"), tok(TokenType::RIGHT_BRACE, "}"),
        eof()
    }), std::exception);
}

TEST(FunctionParserTest, FuncMissingLeftBrace_Throws) {
    // Func foo()  (no brace)
    Parser p;
    EXPECT_THROW(p.parse({
        tok(TokenType::FUN, "Func"), id("foo"),
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
    body.push_back(std::make_unique<PrintStmt>(
        std::make_unique<LiteralExpr>(std::string("hello"), 1), 1));
    stmts.push_back(makeFuncDecl("greet", {}, std::move(body)));
    stmts.push_back(makeCallStmt("greet", {}));
    EXPECT_EQ(runStmts(), "hello\n");
}

// Func add(a, b) { return a + b; }   var r = add(3, 7);   print r;   -> "10\n"
TEST_F(FunctionExecutorTest, FuncCall_ReturnsValue) {
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "a")),
            tok(TokenType::PLUS, "+"),
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "b")))));
    stmts.push_back(makeFuncDecl("add", {"a", "b"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(3.0, 1));
    args.push_back(std::make_unique<LiteralExpr>(7.0, 1));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "r"),
        makeCallExpr("add", std::move(args))));
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "r")), 1));
    EXPECT_EQ(runStmts(), "10\n");
}

// Func noop() {}   var r = noop();   print r;   -> "nil\n"
TEST_F(FunctionExecutorTest, FuncCall_ReturnsNilWhenNoReturn) {
    stmts.push_back(makeFuncDecl("noop", {}, {}));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "r"),
        makeCallExpr("noop", {})));
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "r")), 1));
    EXPECT_EQ(runStmts(), "nil\n");
}

// Func ret_nil() { return; }   var r = ret_nil();   print r;   -> "nil\n"
TEST_F(FunctionExecutorTest, FuncCall_ExplicitReturnNil) {
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<ReturnStmt>(tok(TokenType::RETURN, "return")));
    stmts.push_back(makeFuncDecl("ret_nil", {}, std::move(body)));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "r"),
        makeCallExpr("ret_nil", {})));
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "r")), 1));
    EXPECT_EQ(runStmts(), "nil\n");
}

// Func dbl(x) { return x * 2; }   print dbl(5);   -> "10\n"
TEST_F(FunctionExecutorTest, FuncCall_WithParameters) {
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "x")),
            tok(TokenType::STAR, "*"),
            std::make_unique<LiteralExpr>(2.0, 1))));
    stmts.push_back(makeFuncDecl("dbl", {"x"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(5.0, 1));
    stmts.push_back(std::make_unique<PrintStmt>(
        makeCallExpr("dbl", std::move(args)), 1));
    EXPECT_EQ(runStmts(), "10\n");
}

// Global var a=999 must not be modified by function param a
TEST_F(FunctionExecutorTest, FuncCall_ParamsAreLocal) {
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "a")),
            tok(TokenType::PLUS, "+"),
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "b")))));
    stmts.push_back(makeFuncDecl("add", {"a", "b"}, std::move(body)));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "a"),
        std::make_unique<LiteralExpr>(999.0, 1)));

    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(3.0, 1));
    args.push_back(std::make_unique<LiteralExpr>(7.0, 1));
    stmts.push_back(std::make_unique<ExpressionStmt>(makeCallExpr("add", std::move(args))));
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "a")), 1));
    EXPECT_EQ(runStmts(), "999\n");
}

// ════════════════════════════════════════════════════
// Wave 6 — Executor: Recursion
// ════════════════════════════════════════════════════

// Func fact(n) { if (n <= 1) return 1; return n * fact(n-1); }
// print fact(5);  -> "120\n"
TEST_F(FunctionExecutorTest, FuncCall_Recursive_Factorial) {
    // if (n <= 1) return 1;
    auto ifCond = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "n")),
        tok(TokenType::LESS_EQUAL, "<="),
        std::make_unique<LiteralExpr>(1.0, 1));
    auto ifThen = std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<LiteralExpr>(1.0, 1));

    // return n * fact(n - 1);
    std::vector<ExprPtr> recArgs;
    recArgs.push_back(std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "n")),
        tok(TokenType::MINUS, "-"),
        std::make_unique<LiteralExpr>(1.0, 1)));
    auto retRec = std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(tok(TokenType::IDENTIFIER, "n")),
            tok(TokenType::STAR, "*"),
            makeCallExpr("fact", std::move(recArgs))));

    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<IfStmt>(std::move(ifCond), std::move(ifThen)));
    body.push_back(std::move(retRec));
    stmts.push_back(makeFuncDecl("fact", {"n"}, std::move(body)));

    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(5.0, 1));
    stmts.push_back(std::make_unique<PrintStmt>(
        makeCallExpr("fact", std::move(args)), 1));
    EXPECT_EQ(runStmts(), "120\n");
}

// ════════════════════════════════════════════════════
// Wave 7 — Executor: Error Cases
// ════════════════════════════════════════════════════

// Func foo(a) {}   foo(1, 2);   -> throws (too many args)
TEST_F(FunctionExecutorTest, FuncCall_TooManyArgs_Throws) {
    stmts.push_back(makeFuncDecl("foo", {"a"}, {}));
    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(1.0, 1));
    args.push_back(std::make_unique<LiteralExpr>(2.0, 1));
    stmts.push_back(makeCallStmt("foo", std::move(args)));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// Func foo(a, b) {}   foo(1);   -> throws (too few args)
TEST_F(FunctionExecutorTest, FuncCall_TooFewArgs_Throws) {
    stmts.push_back(makeFuncDecl("foo", {"a", "b"}, {}));
    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(1.0, 1));
    stmts.push_back(makeCallStmt("foo", std::move(args)));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// var x = "hello";   x();   -> throws (not a function)
TEST_F(FunctionExecutorTest, FuncCall_NonCallableVariable_Throws) {
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        tok(TokenType::IDENTIFIER, "x"),
        std::make_unique<LiteralExpr>(std::string("hello"), 1)));
    stmts.push_back(makeCallStmt("x", {}));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// undefinedFunc();   -> throws
TEST_F(FunctionExecutorTest, FuncCall_UndefinedFunction_Throws) {
    stmts.push_back(makeCallStmt("undefinedFunc", {}));
    EXPECT_THROW(runStmts(), std::runtime_error);
}

// ════════════════════════════════════════════════════
// Wave 8 — Checker: Valid Cases
// ════════════════════════════════════════════════════

// Func foo() {}  -> no throw
TEST(FunctionCheckerTest, FuncDecl_NoThrow) {
    Checker checker;
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<FunctionStmt>(
        tok(TokenType::IDENTIFIER, "foo"),
        std::vector<Token>{},
        std::vector<StmtPtr>{}));
    EXPECT_NO_THROW(checker.check(stmts));
}

// Func foo() { return 1; }  -> no throw
TEST(FunctionCheckerTest, ReturnInsideFunction_NoThrow) {
    Checker checker;
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<LiteralExpr>(1.0, 1)));
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<FunctionStmt>(
        tok(TokenType::IDENTIFIER, "foo"),
        std::vector<Token>{},
        std::move(body)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// Func foo() { { return 1; } }  -> no throw (nested block inside function)
TEST(FunctionCheckerTest, ReturnInsideNestedBlock_NoThrow) {
    Checker checker;
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return"),
        std::make_unique<LiteralExpr>(1.0, 1)));
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<FunctionStmt>(
        tok(TokenType::IDENTIFIER, "foo"),
        std::vector<Token>{},
        std::move(body)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 9 — Checker: Error Cases
// ════════════════════════════════════════════════════

// return 5;  (global scope)  -> throws CheckError
TEST(FunctionCheckerTest, ReturnOutsideFunction_Throws) {
    Checker checker;
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return", {}, 1),
        std::make_unique<LiteralExpr>(5.0, 1)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// return; inside block but outside function  -> throws CheckError
TEST(FunctionCheckerTest, ReturnInsideBlock_OutsideFunction_Throws) {
    Checker checker;
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return", {}, 1)));
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// Func foo(a, a) {}  -> throws CheckError
TEST(FunctionCheckerTest, DuplicateParams_Throws) {
    Checker checker;
    std::vector<Token> params = {
        tok(TokenType::IDENTIFIER, "a", {}, 1),
        tok(TokenType::IDENTIFIER, "a", {}, 1)
    };
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<FunctionStmt>(
        tok(TokenType::IDENTIFIER, "foo"),
        std::move(params),
        std::vector<StmtPtr>{}));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// Duplicate param error message contains the param name
TEST(FunctionCheckerTest, DuplicateParams_ErrorContainsName) {
    Checker checker;
    std::vector<Token> params = {
        tok(TokenType::IDENTIFIER, "x", {}, 1),
        tok(TokenType::IDENTIFIER, "x", {}, 2)
    };
    std::vector<StmtPtr> stmts;
    stmts.push_back(std::make_unique<FunctionStmt>(
        tok(TokenType::IDENTIFIER, "foo"),
        std::move(params),
        std::vector<StmtPtr>{}));
    try {
        checker.check(stmts);
        FAIL() << "Expected CheckError";
    } catch (const CheckError& e) {
        EXPECT_NE(std::string(e.what()).find("x"), std::string::npos);
    }
}
