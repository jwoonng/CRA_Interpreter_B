#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// ── File mode tests (factory "run <path>") ────────────────────────────
// Verifies Shell::runFile: whole-file execution, file-not-found handling,
// and immediate stop with a line-numbered message on runtime error.

namespace {

// Write content to a unique temp file and return its path.
std::string writeTempScript(const std::string& content) {
    namespace fs = std::filesystem;
    static int counter = 0;
    fs::path p = fs::temp_directory_path() /
                 ("codefab_filemode_" + std::to_string(++counter) + ".txt");
    std::ofstream(p) << content;
    return p.string();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

TEST(FileModeTest, MissingFile_ReturnsErrorCode) {
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile("no_such_file_12345.txt", out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "cannot open file"));
}

TEST(FileModeTest, SimpleProgram_ProducesOutput) {
    std::string path = writeTempScript("print 1 + 2;\n");
    Shell shell;
    std::ostringstream out;
    int code = shell.runFile(path, out);
    EXPECT_EQ(code, 0);
    EXPECT_EQ(out.str(), "3\n");
}

TEST(FileModeTest, MultipleStatements_RunInOrder) {
    std::string path = writeTempScript(
        "var a = 10;\n"
        "var b = 20;\n"
        "print a + b;\n");
    Shell shell;
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "30\n");
}

TEST(FileModeTest, RuntimeError_ReportsLineAndStops) {
    std::string path = writeTempScript(
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
    std::string path = writeTempScript(
        "func add(a, b) { return a + b; }\n"
        "var arr = array(2);\n"
        "arr[0] = add(3, 4);\n"
        "print arr[0];\n");
    Shell shell;
    std::ostringstream out;
    EXPECT_EQ(shell.runFile(path, out), 0);
    EXPECT_EQ(out.str(), "7\n");
}
