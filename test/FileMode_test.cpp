#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "FileTestHelpers.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
#include <sstream>
#include <string>

// ── File mode tests (factory "run <path>") ────────────────────────────
// Verifies Shell::runFile: whole-file execution, file-not-found handling,
// and immediate stop with a line-numbered message on runtime error.

TEST(FileModeTest, MissingFile_ReturnsErrorCode) {
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile("no_such_file_12345.txt", out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "cannot open file"));
}

TEST(FileModeTest, SimpleProgram_ProducesOutput) {
    auto path = writeTempScript("filemode", "print 1 + 2;\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 0);
    EXPECT_EQ(out.str(), "3\n");
}

TEST(FileModeTest, MultipleStatements_RunInOrder) {
    auto path = writeTempScript("filemode",
        "var a = 10;\n"
        "var b = 20;\n"
        "print a + b;\n");
    Shell shell;
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "30\n");
}

TEST(FileModeTest, RuntimeError_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "print 1;\n"
        "print 1 / 0;\n"   // runtime error on line 2
        "print 3;\n");     // must NOT run
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_TRUE(contains(out.str(), "Division by zero"));
    EXPECT_FALSE(contains(out.str(), "3\n"));  // stopped before line 3
}

TEST(FileModeTest, FunctionAndArray_WorkFromFile) {
    auto path = writeTempScript("filemode",
        "func add(a, b) { return a + b; }\n"
        "var arr = array(2);\n"
        "arr[0] = add(3, 4);\n"
        "print arr[0];\n");
    Shell shell;
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "7\n");
}

// Checker 오류(중복 선언)는 실행을 막고 에러 코드 1을 반환한다
TEST(FileModeTest, CheckerError_StopsExecutionAndReturnsOne) {
    auto path = writeTempScript("filemode",
        "{ var a = 1; var a = 2; }\n"
        "print 42;\n");  // 절대 실행되면 안 된다
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "already declared"));
    EXPECT_FALSE(contains(out.str(), "42"));
}

// Shell에 옵티마이저가 추가된 상태에서도 runFile이 올바른 결과를 낸다
// (Shell::runFile의 optimizer 루프 경로 커버)
TEST(FileModeTest, WithOptimizer_RunFileProducesCorrectOutput) {
    auto path = writeTempScript("filemode", "print 2 + 3;\n");
    Shell shell;
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "5\n");
}

// ── 빈 파일 ────────────────────────────────────────────────────────────

TEST(FileModeTest, EmptyFile_ReturnsZeroWithNoOutput) {
    auto path = writeTempScript("filemode", "");
    Shell shell;
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "");
}

// ── 배열 런타임 오류 — 줄 번호 포함 출력 후 즉시 종료 ──────────────────

// arr[5] — 크기 3인 배열의 범위 초과 인덱스
TEST(FileModeTest, RuntimeError_ArrayIndexOutOfBounds_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "var arr = array(3);\n"
        "print arr[5];\n"        // 런타임 오류: 줄 2
        "print \"FAIL\";\n");    // 즉시 종료 검증 — 출력되면 안 됨
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// arr["hello"] — 인덱스가 숫자가 아닌 경우
TEST(FileModeTest, RuntimeError_NonNumericIndex_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "var arr = array(3);\n"
        "print arr[\"hello\"];\n"  // 런타임 오류: 줄 2
        "print \"FAIL\";\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// var x = 10; x[0] — 배열이 아닌 대상에 [] 사용
TEST(FileModeTest, RuntimeError_IndexOnNonArray_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "var x = 10;\n"
        "print x[0];\n"          // 런타임 오류: 줄 2
        "print \"FAIL\";\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// array("hi") — 배열 크기로 숫자가 아닌 값
TEST(FileModeTest, RuntimeError_NonNumericArraySize_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "var arr = array(\"hi\");\n"  // 런타임 오류: 줄 1
        "print \"FAIL\";\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 1]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// ── 함수 런타임 오류 — 줄 번호 포함 출력 후 즉시 종료 ─────────────────

// var notFunc = "hello"; notFunc() — 함수가 아닌 대상 호출
TEST(FileModeTest, RuntimeError_CallNonFunction_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "var notFunc = \"hello\";\n"
        "notFunc();\n"            // 런타임 오류: 줄 2
        "print \"FAIL\";\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// func foo(a,b,c) {} foo(1,2) — 인자 개수 불일치 (3개 파라미터, 2개 인자)
TEST(FileModeTest, RuntimeError_ArgCountMismatch_ReportsLineAndStops) {
    auto path = writeTempScript("filemode",
        "func foo(a, b, c) { return a + b + c; }\n"
        "foo(1, 2);\n"            // 런타임 오류: 줄 2
        "print \"FAIL\";\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "[line 2]"));
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// ── Checker(정적) 오류 — 실행 전 검출, 즉시 종료 ─────────────────────

// return 5; (전역 스코프) — Checker가 실행 전에 검출
TEST(FileModeTest, CheckerError_ReturnOutsideFunction_StopsBeforeExecution) {
    auto path = writeTempScript("filemode",
        "return 5;\n"
        "print \"FAIL\";\n");    // Checker 오류로 실행 자체가 시작되지 않음
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}

// func foo(a, a) {} — 파라미터 이름 중복, Checker가 실행 전에 검출
TEST(FileModeTest, CheckerError_DuplicateParams_StopsBeforeExecution) {
    auto path = writeTempScript("filemode",
        "func foo(a, a) { return a; }\n"
        "print \"FAIL\";\n");    // Checker 오류로 실행 자체가 시작되지 않음
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 1);
    EXPECT_FALSE(contains(out.str(), "FAIL"));
}
