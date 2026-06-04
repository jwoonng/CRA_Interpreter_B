#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/checker/Checker.h"

using namespace testing;

// Checker Mock
class MockChecker : public IChecker {
public:
    MOCK_METHOD(void, check, (const std::vector<std::unique_ptr<Stmt>>&), (override));
};

// Default -> PASS
TEST(CheckerTest, ValidCodeNoThrow) {
    Checker checker;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_NO_THROW(checker.check(stmts));
}

// 다른 블록 스코프에서 같은 이름 변수 재선언(shadowing) -> 예외 없이 허용
TEST(CheckerTest, ShadowingAllowed) {
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_)).Times(1);
    EXPECT_NO_THROW(mock.check(stmts));
}

// 초기화식에서 선언 중인 자기 자신을 참조 -> CheckError 발생
TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_))
        .WillOnce(Throw(CheckError(1, "자신의 초기화식에서 지역변수 'x'을(를) 읽을 수 없습니다.")));
    EXPECT_THROW(mock.check(stmts), std::runtime_error);
}

// 같은 블록 스코프에서 동일 이름 변수 중복 선언 -> CheckError 발생
TEST(CheckerTest, DuplicateVarInSameScopeThrows) {
    MockChecker mock;
    std::vector<std::unique_ptr<Stmt>> stmts;
    EXPECT_CALL(mock, check(_))
        .WillOnce(Throw(CheckError(2, "변수 'x'이(가) 이미 이 블록에서 선언되었습니다.")));
    EXPECT_THROW(mock.check(stmts), std::runtime_error);
}
