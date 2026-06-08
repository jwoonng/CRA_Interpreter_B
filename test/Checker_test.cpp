#include <gtest/gtest.h>
#include "src/checker/Checker.h"
#include "TestHelpers.h"

class CheckerTest : public ::testing::Test {
protected:
    Checker checker;
};

// ════════════════════════════════════════════════════
// Wave 1 — 기본 동작 (빈 프로그램, 글로벌 선언, 리터럴)
// 목표: Checker가 정상 코드에서 예외를 던지지 않음
// ════════════════════════════════════════════════════

TEST_F(CheckerTest, EmptyProgramNoThrow) {
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, GlobalVarDeclNoThrow) {
    // var x = 1;  — 글로벌 스코프: scopes_가 비어있어 중복 검사 생략
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        id("x"), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, GlobalVarDeclWithoutInitializerNoThrow) {
    // var x;  — 초기화식 없는 글로벌 선언
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(id("x")));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, LiteralExprStmtNoThrow) {
    // 42;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LiteralExpr>(42.0, 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

// ════════════════════════════════════════════════════
// Wave 2 — 블록 스코프 (지역 변수, 섀도잉, 중복 선언)
// ════════════════════════════════════════════════════

TEST_F(CheckerTest, LocalVarDeclInBlockNoThrow) {
    // { var x = 1; }
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, ShadowingInNestedBlockAllowed) {
    // { var x = 1; { var x = 2; } }  — 다른 스코프면 같은 이름 허용
    std::vector<StmtPtr> innerStmts;
    innerStmts.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    auto innerBlock = std::make_unique<BlockStmt>(std::move(innerStmts));
    std::vector<StmtPtr> outerStmts;
    outerStmts.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    outerStmts.push_back(std::move(innerBlock));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outerStmts)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, DuplicateVarInSameScopeThrows) {
    // { var x = 1; var x = 2; }  — 같은 블록 중복 선언 → CheckError
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

TEST_F(CheckerTest, DuplicateVarErrorContainsLineAndName) {
    // 오류 메시지에 라인 번호와 변수명이 포함되어야 함
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    try {
        checker.check(stmts);
        FAIL() << "Expected CheckError to be thrown";
    }
    catch (const CheckError& e) {
        EXPECT_EQ(e.line, 2);
        EXPECT_NE(std::string(e.what()).find("x"), std::string::npos);
    }
}

TEST_F(CheckerTest, OuterVarAccessibleInInnerBlockNoThrow) {
    // { var x = 1; { print x; } }  — 외부 스코프 변수를 내부에서 접근
    Token xTok = id("x", 1);
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

TEST_F(CheckerTest, SelfReferenceInInitializerThrows) {
    // { var x = x; }  — 초기화식에서 자기 자신을 참조 → CheckError
    Token xTok = id("x", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        xTok, std::make_unique<VariableExpr>(xTok)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

TEST_F(CheckerTest, SelfReferenceErrorContainsLineAndName) {
    // 자기 참조 오류 메시지에 라인 번호와 변수명 포함
    Token xTok = id("x", 3);
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
        EXPECT_NE(std::string(e.what()).find("x"), std::string::npos);
    }
}

// ════════════════════════════════════════════════════
// Wave 4 — 표현식 유형별 검사 (BinaryExpr, UnaryExpr, GroupingExpr, LogicalExpr, AssignExpr)
// ════════════════════════════════════════════════════

TEST_F(CheckerTest, BinaryExprWithDeclaredVarsNoThrow) {
    // { var a = 1; var b = a + 1; }
    Token aTok = id("a", 1);
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(
        aTok, std::make_unique<LiteralExpr>(1.0, 1)
    ));
    auto addExpr = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(aTok),
        tok(TokenType::PLUS, "+"),
        std::make_unique<LiteralExpr>(1.0, 1)
    );
    inner.push_back(std::make_unique<VarDeclareStmt>(
        id("b", 1), std::move(addExpr)
    ));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(inner)));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, UnaryMinusExprNoThrow) {
    // -42;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<UnaryExpr>(
            tok(TokenType::MINUS, "-"),
            std::make_unique<LiteralExpr>(42.0, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, UnaryBangExprNoThrow) {
    // !true;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<UnaryExpr>(
            tok(TokenType::BANG, "!"),
            std::make_unique<LiteralExpr>(true, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, GroupingExprNoThrow) {
    // (1 + 2);
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<GroupingExpr>(
            std::make_unique<BinaryExpr>(
                std::make_unique<LiteralExpr>(1.0, 1),
                tok(TokenType::PLUS, "+"),
                std::make_unique<LiteralExpr>(2.0, 1)
            )
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, LogicalAndExprNoThrow) {
    // true and false;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LogicalExpr>(
            std::make_unique<LiteralExpr>(true, 1),
            tok(TokenType::AND, "and"),
            std::make_unique<LiteralExpr>(false, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, LogicalOrExprNoThrow) {
    // false or true;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ExpressionStmt>(
        std::make_unique<LogicalExpr>(
            std::make_unique<LiteralExpr>(false, 1),
            tok(TokenType::OR, "or"),
            std::make_unique<LiteralExpr>(true, 1)
        )
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, AssignExprNoThrow) {
    // { var x = 1; x = 2; }  — 값 할당 검사 (대상 이름은 Executor가 처리)
    Token xTok = id("x", 1);
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

TEST_F(CheckerTest, PrintLiteralNoThrow) {
    // print 42;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<PrintStmt>(
        std::make_unique<LiteralExpr>(42.0, 1), 1
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, PrintVarInBlockNoThrow) {
    // { var x = 1; print x; }
    Token xTok = id("x", 1);
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

TEST_F(CheckerTest, IfStmtNoThrow) {
    // if (true) print 1;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(true, 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, IfElseStmtNoThrow) {
    // if (true) print 1; else print 2;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<LiteralExpr>(true, 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(2.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, IfConditionWithBinaryExprNoThrow) {
    // if (1 < 2) print 1;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<IfStmt>(
        std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(1.0, 1),
            tok(TokenType::LESS, "<"),
            std::make_unique<LiteralExpr>(2.0, 1)
        ),
        std::make_unique<PrintStmt>(std::make_unique<LiteralExpr>(1.0, 1), 1)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, ForStmtNoThrow) {
    // for (var i = 0; i < 3; i = i + 1) print i;
    Token iTok = id("i", 1);
    auto init = std::make_unique<VarDeclareStmt>(
        iTok, std::make_unique<LiteralExpr>(0.0, 1)
    );
    auto cond = std::make_unique<BinaryExpr>(
        std::make_unique<VariableExpr>(iTok),
        tok(TokenType::LESS, "<"),
        std::make_unique<LiteralExpr>(3.0, 1)
    );
    auto incr = std::make_unique<AssignExpr>(
        iTok,
        std::make_unique<BinaryExpr>(
            std::make_unique<VariableExpr>(iTok),
            tok(TokenType::PLUS, "+"),
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

TEST_F(CheckerTest, ForStmtNullInitAndCondNoThrow) {
    // for (; ; ) { }  — 초기자·조건·증가식 모두 null
    std::vector<StmtPtr> bodyStmts;
    auto body = std::make_unique<BlockStmt>(std::move(bodyStmts));
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<ForStmt>(
        nullptr, nullptr, nullptr, std::move(body)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST_F(CheckerTest, ForStmtVarSelfRefInInitThrows) {
    // for (var i = i; ; ) { }  — for 초기화에서 자기 참조
    Token iTok = id("i", 1);
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
// Wave 6 — CheckError 타입 계층 검증
// ════════════════════════════════════════════════════

TEST_F(CheckerTest, CheckErrorIsRuntimeError) {
    // CheckError가 std::runtime_error의 파생 타입임을 검증
    EXPECT_THROW(
        ([]() { throw CheckError(1, "test error"); }()),
        std::runtime_error
    );
}

// ════════════════════════════════════════════════════
// Wave 7 — 글로벌 변수 중복 선언
// ════════════════════════════════════════════════════

TEST(CheckerTest, GlobalDuplicateVarThrows) {
    // var x = 1; var x = 2;  — 글로벌 레벨 중복 선언
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    EXPECT_THROW(checker.check(stmts), CheckError);
}

TEST(CheckerTest, GlobalDuplicateVarErrorMessage) {
    // 에러 메시지에 변수명과 줄번호가 포함되어야 함
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 3), std::make_unique<LiteralExpr>(2.0, 3)
    ));
    try {
        checker.check(stmts);
        FAIL() << "expected throw";
    } catch (const CheckError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("x"),      std::string::npos) << "message: " << msg;
        EXPECT_NE(msg.find("line 3"), std::string::npos) << "message: " << msg;
        EXPECT_NE(msg.find("line 1"), std::string::npos) << "first-decl line in message: " << msg;
    }
}

TEST(CheckerTest, GlobalUniqueVarNoThrow) {
    // var x = 1; var y = 2;  — 이름이 다르면 정상
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("y", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    EXPECT_NO_THROW(checker.check(stmts));
}

TEST(CheckerTest, LocalShadowsGlobalNoThrow) {
    // var x = 1; { var x = 2; }  — 로컬에서 글로벌 이름 재사용은 허용
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1), std::make_unique<LiteralExpr>(1.0, 1)
    ));
    std::vector<StmtPtr> innerStmts;
    innerStmts.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 2), std::make_unique<LiteralExpr>(2.0, 2)
    ));
    stmts.push_back(std::make_unique<BlockStmt>(std::move(innerStmts)));
    EXPECT_NO_THROW(checker.check(stmts));
}
