#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/checker/Checker.h"
#include "test/mocks/MockChecker.h"

using namespace testing;

// ── 헬퍼 ────────────────────────────────────────────
namespace {

    Token tok(TokenType type, std::string lexeme, LiteralValue lit = {}, int line = 1) {
        return Token{ type, std::move(lexeme), std::move(lit), line };
    }
    Token ident(std::string name, int line = 1) {
        return tok(TokenType::IDENTIFIER, std::move(name), {}, line);
    }
    Token numTok(double v, int line = 1) {
        return tok(TokenType::NUMBER, std::to_string(v), v, line);
    }
    Token opTok(TokenType type, std::string lex, int line = 1) {
        return tok(type, std::move(lex), {}, line);
    }

} // namespace

// ════════════════════════════════════════════════════
// Wave 1 — 기본 동작 (빈 프로그램, 글로벌 선언, 리터럴)
// 목표: Checker가 정상 코드에서 예외를 던지지 않음
// ════════════════════════════════════════════════════

TEST(CheckerTest, EmptyProgramNoThrow) {
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, GlobalVarDeclNoThrow) {
    // var x = 1;  — 글로벌 스코프: scopes_가 비어있어 중복 검사 생략
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x"), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, GlobalVarDeclWithoutInitializerNoThrow) {
    // var x;  — 초기화식 없는 글로벌 선언
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(ident("x")));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, LiteralExprStmtNoThrow) {
    // 42;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LiteralExpr>(42.0, 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 2 — 블록 스코프 (지역 변수, 섀도잉, 중복 선언)
// ════════════════════════════════════════════════════

TEST(CheckerTest, LocalVarDeclInBlockNoThrow) {
    // { var x = 1; }
    Checker checker;
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, ShadowingInNestedBlockAllowed) {
    // { var x = 1; { var x = 2; } }  — 다른 스코프면 같은 이름 허용
    Checker checker;
    std::vector<StmtPtr> innerStmts;
    innerStmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStmts));
    std::vector<StmtPtr> outerStmts;
    outerStmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    outerStmts.push_back(std::move(innerBlock));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outerStmts)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, DuplicateVarInSameScopeThrows) {
    // { var x = 1; var x = 2; }  — 같은 블록 중복 선언 → CheckError
    Checker checker;
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

TEST(CheckerTest, DuplicateVarErrorContainsLineAndName) {
    // 오류 메시지에 라인 번호와 변수명이 포함되어야 함
    Checker checker;
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    try {
        checker.check(stmts);
        FAIL() << "Expected CheckError to be thrown";
    }
    catch (const CheckError& e) {
        EXPECT_EQ(e.line, 2);
        EXPECT_THAT(e.what(), HasSubstr("x"));
    }
}

TEST(CheckerTest, OuterVarAccessibleInInnerBlockNoThrow) {
    // { var x = 1; { print x; } }  — 외부 스코프 변수를 내부에서 접근
    Checker checker;
    Token xTok = ident("x", 1);
    std::vector<StmtPtr> innerStmts;
    innerStmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(xTok), 2
    ));
    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStmts));
    std::vector<StmtPtr> outerStmts;
    outerStmts.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<LiteralExpr>(1.0, 1)
    ));
    outerStmts.push_back(std::move(innerBlock));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outerStmts)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 3 — 자기 참조 (var x = x;)
// ════════════════════════════════════════════════════

TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    // { var x = x; }  — 초기화식에서 자기 자신을 참조 → CheckError
    Checker checker;
    Token xTok = ident("x", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<VariableExpr>(xTok)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

TEST(CheckerTest, SelfReferenceErrorContainsLineAndName) {
    // 자기 참조 오류 메시지에 라인 번호와 변수명 포함
    Checker checker;
    Token xTok = ident("x", 3);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<VariableExpr>(xTok)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    try {
        checker.check(stmts);
        FAIL() << "Expected CheckError to be thrown";
    }
    catch (const CheckError& e) {
        EXPECT_EQ(e.line, 3);
        EXPECT_THAT(e.what(), HasSubstr("x"));
    }
}

// ════════════════════════════════════════════════════
// Wave 4 — 표현식 유형별 검사 (BinaryExpr, UnaryExpr, GroupingExpr, LogicalExpr, AssignExpr)
// ════════════════════════════════════════════════════

TEST(CheckerTest, BinaryExprWithDeclaredVarsNoThrow) {
    // { var a = 1; var b = a + 1; }
    Checker checker;
    Token aTok = ident("a", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        aTok, std::make_unique<LiteralExpr>(1.0, 1)
    ));
    auto addExpr = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(aTok),
        opTok(TokenType::PLUS, "+"),
        std::make_unique<LiteralExpr>(1.0, 1)
    );
    inner.push_back(std::make_unique<VarDeclareStmt>(
        ident("b", 1), std::move(addExpr)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, UnaryMinusExprNoThrow) {
    // -42;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<UnaryExpr>(
            opTok(TokenType::MINUS, "-"),
            std::make_unique<LiteralExpr>(42.0, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, UnaryBangExprNoThrow) {
    // !true;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<UnaryExpr>(
            opTok(TokenType::BANG, "!"),
            std::make_unique<LiteralExpr>(true, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, GroupingExprNoThrow) {
    // (1 + 2);
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<GroupingExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<LiteralExpr>(1.0, 1),
                opTok(TokenType::PLUS, "+"),
                std::make_unique<LiteralExpr>(2.0, 1)
            )
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, LogicalAndExprNoThrow) {
    // true and false;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LogicalExpr>(
            std::make_unique<LiteralExpr>(true, 1),
            opTok(TokenType::AND, "and"),
            std::make_unique<LiteralExpr>(false, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, LogicalOrExprNoThrow) {
    // false or true;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LogicalExpr>(
            std::make_unique<LiteralExpr>(false, 1),
            opTok(TokenType::OR, "or"),
            std::make_unique<LiteralExpr>(true, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, AssignExprNoThrow) {
    // { var x = 1; x = 2; }  — 값 할당 검사 (대상 이름은 Executor가 처리)
    Checker checker;
    Token xTok = ident("x", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<AssignExpr>(
            xTok,
            std::make_unique<LiteralExpr>(2.0, 1)
        )
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 5 — Statement 유형별 검사 (PrintStmt, IfStmt, ForStmt)
// ════════════════════════════════════════════════════

TEST(CheckerTest, PrintLiteralNoThrow) {
    // print 42;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<LiteralExpr>(42.0, 1), 1
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, PrintVarInBlockNoThrow) {
    // { var x = 1; print x; }
    Checker checker;
    Token xTok = ident("x", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(xTok), 1
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, IfStmtNoThrow) {
    // if (true) print 1;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(true, 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, IfElseStmtNoThrow) {
    // if (true) print 1; else print 2;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(true, 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(2.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, IfConditionWithBinaryExprNoThrow) {
    // if (1 < 2) print 1;
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(1.0, 1),
            opTok(TokenType::LESS, "<"),
            std::make_unique<LiteralExpr>(2.0, 1)
        ),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, ForStmtNoThrow) {
    // for (var i = 0; i < 3; i = i + 1) print i;
    Checker checker;
    Token iTok = ident("i", 1);
    auto init = std::make_unique<VarDeclareStmt>(
        iTok, std::make_unique<LiteralExpr>(0.0, 1)
    );
    auto cond = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(iTok),
        opTok(TokenType::LESS, "<"),
        std::make_unique<LiteralExpr>(3.0, 1)
    );
    auto incr = std::make_unique<AssignExpr>(
        iTok,
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(iTok),
            opTok(TokenType::PLUS, "+"),
            std::make_unique<LiteralExpr>(1.0, 1)
        )
    );
    auto body = std::make_unique<PrintStmt>(
        std::make_unique<VariableExpr>(iTok), 1
    );
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ForStmt>(
        std::move(init), std::move(cond), std::move(incr), std::move(body)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, ForStmtNullInitAndCondNoThrow) {
    // for (; ; ) { }  — 초기자·조건·증가식 모두 null
    Checker checker;
    std::vector<StmtPtr> bodyStmts;
    auto body = std::make_unique<BlockStmt>(std::move(bodyStmts));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ForStmt>(
        nullptr, nullptr, nullptr, std::move(body)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, ForStmtVarSelfRefInInitThrows) {
    // for (var i = i; ; ) { }  — for 초기화에서 자기 참조
    Checker checker;
    Token iTok = ident("i", 1);
    auto init = std::make_unique<VarDeclareStmt>(
        iTok, std::make_unique<VariableExpr>(iTok)
    );
    std::vector<StmtPtr> bodyStmts;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ForStmt>(
        std::move(init), nullptr, nullptr,
        std::make_unique<BlockStmt>(std::move(bodyStmts))
    ));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

// ════════════════════════════════════════════════════
// Wave 6 — Mock 인터페이스 테스트 (IChecker 계약 검증)
// ════════════════════════════════════════════════════

TEST(CheckerMockTest, MockCheckerCallsCheckOnce) {
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_)).Times(1);
    EXPECT_NO_THROW(mock.check(stmts));
}

TEST(CheckerMockTest, MockCheckerCanThrowCheckError) {
    // MockChecker can throw CheckError (self-reference scenario)
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_))
        .WillOnce(Throw(CheckError(1, "self-reference in initializer: 'x'")));
    EXPECT_THROW(mock.check(stmts), CheckError);
}

TEST(CheckerMockTest, MockCheckerCanThrowForDuplicate) {
    // MockChecker can throw CheckError for duplicate declaration
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_))
        .WillOnce(Throw(CheckError(2, "variable 'x' already declared in this scope")));
    EXPECT_THROW(mock.check(stmts), CheckError);
}

TEST(CheckerMockTest, CheckErrorIsRuntimeError) {
    // CheckError is a subtype of std::runtime_error
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_))
        .WillOnce(Throw(CheckError(1, "test error")));
    EXPECT_THROW(mock.check(stmts), std::runtime_error);
}
