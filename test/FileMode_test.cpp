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
