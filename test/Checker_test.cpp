#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/checker/Checker.h"

using namespace testing;

// IChecker 인터페이스의 gmock 테스트 대역
class MockChecker : public IChecker {
public:
    MOCK_METHOD(void, check, (const std::vector<std::unique_ptr<Stmt>>&), (override));
};

// ── 테스트 헬퍼 ──────────────────────────────────────────────────────────────

// IDENTIFIER 토큰 생성
static Token ident(const std::string& name, int line = 1) {
    return Token{TokenType::IDENTIFIER, name, {}, line};
}

// 숫자 리터럴 Expr 생성
static ExprPtr numLit(double v, int line = 1) {
    return std::make_unique<LiteralExpr>(LiteralValue{v}, line);
}

// ── 개발자 3 담당 영역 ──────────────────────────────

// 빈 AST → 예외 없이 통과
TEST(CheckerTest, ValidCodeNoThrow) {
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_NO_THROW(checker.check(stmts));
}

// 다른 블록 스코프에서의 같은 이름 재선언(shadowing) → 허용
// { var x = 1; { var x = 2; } }
TEST(CheckerTest, ShadowingAllowed) {
    Checker checker;

    // 내부 블록: { var x = 2; }
    std::vector<StmtPtr> inner;
    inner.push_back(std::make_unique<VarDeclareStmt>(ident("x", 2), numLit(2.0, 2)));

    // 외부 블록: { var x = 1; { var x = 2; } }
    std::vector<StmtPtr> outer;
    outer.push_back(std::make_unique<VarDeclareStmt>(ident("x", 1), numLit(1.0, 1)));
    outer.push_back(std::make_unique<BlockStmt>(std::move(inner)));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(outer)));

    EXPECT_NO_THROW(checker.check(stmts));
}

// 초기화식에서 선언 중인 자기 자신 참조 → CheckError 발생
// { var x = x; }
TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    Checker checker;

    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1),
        std::make_unique<VariableExpr>(ident("x", 1))  // 초기화식에서 x 참조
    ));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(body)));

    EXPECT_THROW(checker.check(stmts), CheckError);
}

// 같은 블록 스코프에서 동일 이름 변수 중복 선언 → CheckError 발생
// { var x = 1; var x = 2; }
TEST(CheckerTest, DuplicateVarInSameScopeThrows) {
    Checker checker;

    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<VarDeclareStmt>(ident("x", 1), numLit(1.0, 1)));
    body.push_back(std::make_unique<VarDeclareStmt>(ident("x", 2), numLit(2.0, 2)));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(body)));

    EXPECT_THROW(checker.check(stmts), CheckError);
}

// 초기화식 안에서 자기 자신에 대입 → CheckError 발생
// { var x = (x = 5); }  — x가 미초기화 상태일 때 대입 대상으로 사용
TEST(CheckerTest, AssignToUninitializedVarInInitializerThrows) {
    Checker checker;

    // var x = (x = 5);
    std::vector<StmtPtr> body;
    body.push_back(std::make_unique<VarDeclareStmt>(
        ident("x", 1),
        std::make_unique<AssignExpr>(ident("x", 1), numLit(5.0, 1))
    ));

    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.push_back(std::make_unique<BlockStmt>(std::move(body)));

    EXPECT_THROW(checker.check(stmts), CheckError);
}
