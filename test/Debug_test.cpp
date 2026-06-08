#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "FileTestHelpers.h"
#include <sstream>
#include <string>

// ── Debug mode tests (factory "debug <path>") ─────────────────────────
// Drives Shell::runDebug with a scripted command stream and inspects the
// debugger's output: stepping, breakpoints, watch variables, inspect.

namespace {

// Run debug mode over `source` feeding `commands`, return everything printed.
std::string runDebug(const std::string& source, const std::string& commands) {
    auto path = writeTempScript("debug", source);
    Shell shell;
    std::istringstream in(commands);
    std::ostringstream out;
    shell.runDebug(path, in, out);
    return out.str();
}

}  // namespace

TEST(DebugModeTest, MissingFile_ReturnsErrorCode) {
    Shell shell;
    std::istringstream in("");
    std::ostringstream out;
    int code = shell.runDebug("no_such_file_98765.txt", in, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "cannot open file"));
}

TEST(DebugModeTest, StopsAtFirstStatementOnLoad) {
    std::string out = runDebug(
        "var a = 3;\n"
        "print a;\n",
        "continue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] loaded source:"));
    EXPECT_TRUE(contains(out, "[DEBUG] stopped at line 1 -> var a = 3;"));
}

TEST(DebugModeTest, StepWalksStatementByStatement) {
    std::string out = runDebug(
        "var a = 3;\n"
        "var b = 4;\n"
        "print a + b;\n",
        "step\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "stopped at line 1 -> var a = 3;"));
    EXPECT_TRUE(contains(out, "stopped at line 2 -> var b = 4;"));
    EXPECT_TRUE(contains(out, "stopped at line 3 -> print a + b;"));
    EXPECT_TRUE(contains(out, "7\n"));
    EXPECT_TRUE(contains(out, "[DEBUG] execution finished"));
}

TEST(DebugModeTest, BreakpointThenContinue) {
    std::string out = runDebug(
        "var a = 1;\n"
        "var b = 2;\n"
        "var c = 3;\n"
        "print c;\n",
        "break 4\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] breakpoint set at line 4"));
    EXPECT_TRUE(contains(out, "stopped at line 4 (breakpoint) -> print c;"));
    EXPECT_TRUE(contains(out, "3\n"));
}

TEST(DebugModeTest, BreakpointsListAndRemove) {
    std::string out = runDebug(
        "var a = 1;\n"
        "print a;\n",
        "break 2\nbreakpoints\nremove 2\nbreakpoints\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] breakpoints: 2"));
    EXPECT_TRUE(contains(out, "[DEBUG] breakpoint removed at line 2"));
    EXPECT_TRUE(contains(out, "[DEBUG] no breakpoints set"));
}

TEST(DebugModeTest, WatchReportsValueAtEachStop) {
    std::string out = runDebug(
        "var a = 3;\n"
        "a = a + 1;\n"
        "print a;\n",
        "watch a\nstep\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "[WATCH] now watching 'a'"));
    EXPECT_TRUE(contains(out, "[WATCH] a = 3"));  // before a = a + 1
    EXPECT_TRUE(contains(out, "[WATCH] a = 4"));  // after a = a + 1
}

TEST(DebugModeTest, UnwatchStopsReporting) {
    std::string out = runDebug(
        "var a = 1;\n"
        "var b = 2;\n"
        "print a;\n",
        "watch a\nunwatch a\nstep\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "[WATCH] stopped watching 'a'"));
}

TEST(DebugModeTest, WatchesCommand_EmptyList_ShowsNoWatches) {
    std::string out = runDebug(
        "var a = 1;\n"
        "print a;\n",
        "watches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] no watched variables"));
}

TEST(DebugModeTest, WatchesCommand_ListsAllWatched) {
    std::string out = runDebug(
        "var x = 7;\n"
        "var y = 3;\n"
        "print x;\n",
        "watch x\nwatch y\nstep\nstep\nwatches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] x = 7"));
    EXPECT_TRUE(contains(out, "[WATCH] y = 3"));
}

TEST(DebugModeTest, InspectShowsLocalAndGlobalScopes) {
    // Pause at "print b;" (line 4) where b is a local in the block and
    // a is a global — inspect must label each scope.
    std::string out = runDebug(
        "var a = 4;\n"
        "{\n"
        "  var b = 10;\n"
        "  print b;\n"
        "}\n",
        "step\nstep\ninspect\nstep\n");
    EXPECT_TRUE(contains(out, "--- current scope variables ---"));
    EXPECT_TRUE(contains(out, "[local] b = 10 (Number)"));
    EXPECT_TRUE(contains(out, "[global] a = 4 (Number)"));
}

TEST(DebugModeTest, NextStepsOverBlockBody) {
    // "next" at the for statement (depth 0) must run the whole loop without
    // pausing inside it, then stop at the following top-level statement.
    std::string out = runDebug(
        "var s = 0;\n"
        "for (var i = 0; i < 3; i = i + 1) { s = s + i; }\n"
        "print s;\n",
        "step\nnext\nstep\n");
    EXPECT_TRUE(contains(out, "stopped at line 2 -> "));
    // After "next" over the loop, the next pause is the print on line 3,
    // never the loop body (line 2's "s = s + i").
    EXPECT_TRUE(contains(out, "stopped at line 3 -> print s;"));
    EXPECT_TRUE(contains(out, "3\n"));  // 0 + 1 + 2
}
