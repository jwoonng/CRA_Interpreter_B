#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "FileTestHelpers.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
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

// ════════════════════════════════════════════════════
// 커버리지 보강 — 디버거 커맨드 엣지 케이스
// ════════════════════════════════════════════════════

TEST(DebugModeTest, UnknownCommand_PrintsError) {
    std::string out = runDebug("var a = 1;\n", "bogus_cmd\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] unknown command: bogus_cmd"));
}

TEST(DebugModeTest, BreakWithBadArg_PrintsInvalidLineNumber) {
    std::string out = runDebug("var a = 1;\n", "break notanumber\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] invalid line number: notanumber"));
}

TEST(DebugModeTest, RemoveNonexistentBreakpoint_PrintsNoBreakpoint) {
    std::string out = runDebug("var a = 1;\n", "remove 999\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] no breakpoint at line 999"));
}

TEST(DebugModeTest, RemoveWithBadArg_PrintsInvalidLineNumber) {
    std::string out = runDebug("var a = 1;\n", "remove notanumber\ncontinue\n");
    EXPECT_TRUE(contains(out, "[DEBUG] invalid line number: notanumber"));
}

TEST(DebugModeTest, WatchDuplicate_PrintsAlreadyWatched) {
    std::string out = runDebug("var a = 1;\n", "watch a\nwatch a\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] 'a' is already watched"));
}

TEST(DebugModeTest, UnwatchNotWatched_PrintsError) {
    std::string out = runDebug("var a = 1;\n", "unwatch z\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] 'z' is not watched"));
}

TEST(DebugModeTest, Watches_EmptyList_PrintsNoWatches) {
    std::string out = runDebug("var a = 1;\n", "watches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] no watched variables"));
}

TEST(DebugModeTest, Watches_WithValue_PrintsCurrentValue) {
    std::string out = runDebug(
        "var a = 42;\n"
        "print a;\n",
        "watch a\nstep\nwatches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] a = 42"));
}

TEST(DebugModeTest, WatchUndefinedVariable_ShowsUndefined) {
    // notDefined 는 스크립트에 없는 변수 — debugLookup 실패 → <undefined>
    std::string out = runDebug(
        "var a = 1;\n"
        "var b = 2;\n",
        "watch notDefined\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "[WATCH] notDefined = <undefined>"));
}

TEST(DebugModeTest, RuntimeError_StopsAndReturnsOne) {
    Shell shell;
    auto path = writeTempScript("debug", "print 1 / 0;\n");
    std::istringstream in("continue\n");
    std::ostringstream out;
    int code = shell.runDebug(path, in, out);
    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(out.str(), "Division by zero"));
}

// ════════════════════════════════════════════════════
// 커버리지 보강 — typeName / stringifyValue 경로
// ════════════════════════════════════════════════════

TEST(DebugModeTest, InspectShowsBoolAndStringTypes) {
    // var flag = true; var name = "hello"; 선언 후 inspect → Boolean / String 타입 표시
    std::string out = runDebug(
        "var flag = true;\n"
        "var name = \"hello\";\n"
        "print flag;\n",
        "step\nstep\ninspect\ncontinue\n");
    EXPECT_TRUE(contains(out, "flag = true (Boolean)"));
    EXPECT_TRUE(contains(out, "name = hello (String)"));
}

TEST(DebugModeTest, WatchFloat_ShowsDecimalValue) {
    // formatDouble 의 ostringstream 경로: 정수가 아닌 실수
    std::string out = runDebug(
        "var pi = 3.14;\n"
        "print pi;\n",
        "watch pi\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "[WATCH] pi = 3.14"));
}

TEST(DebugModeTest, WatchArray_ShowsBracketNotation) {
    // stringifyValue(ArrayPtr) 경로
    std::string out = runDebug(
        "var arr = array(2);\n"
        "arr[0] = 10;\n"
        "arr[1] = 20;\n"
        "print arr;\n",
        "watch arr\nstep\nstep\nstep\nstep\n");
    EXPECT_TRUE(contains(out, "[WATCH] arr = [10, 20]"));
}

// typeName(Nil): nil 타입 변수를 inspect하면 "(Nil)" 표시
TEST(DebugModeTest, InspectShowsNilType) {
    std::string out = runDebug(
        "var x;\n"
        "print 0;\n",
        "step\ninspect\ncontinue\n");
    EXPECT_TRUE(contains(out, "x = nil (Nil)"));
}

// typeName(Array): 배열 타입 변수를 inspect하면 "(Array)" 표시
TEST(DebugModeTest, InspectShowsArrayType) {
    std::string out = runDebug(
        "var arr = array(2);\n"
        "print 0;\n",
        "step\ninspect\ncontinue\n");
    EXPECT_TRUE(contains(out, "(Array)"));
}

// cmdWatches 명시적 호출 시 미정의 변수 → else 분기 "[WATCH] ... = <undefined>"
TEST(DebugModeTest, Watches_ExplicitCommand_UndefinedVar_ShowsUndefined) {
    std::string out = runDebug(
        "var a = 1;\n",
        "watch notDefined\nwatches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] notDefined = <undefined>"));
}

// watches 명령으로 배열 변수를 명시적으로 조회했을 때 브래킷 표기 확인
TEST(DebugModeTest, WatchesCommand_Array_ShowsBracketNotation) {
    std::string out = runDebug(
        "var arr = array(2);\n"
        "arr[0] = 10;\n"
        "arr[1] = 20;\n"
        "print arr;\n",
        // arr[1]=20 실행 후 print 앞에서 멈춤 → watches 로 현재값 조회
        "watch arr\nstep\nstep\nstep\nwatches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] arr = [10, 20]"));
}

// watch 등록 직후 (배열 선언만 된 상태) watches 조회 → [nil, nil]
TEST(DebugModeTest, WatchesCommand_Array_BeforeAssign_ShowsNil) {
    std::string out = runDebug(
        "var arr = array(2);\n"
        "arr[0] = 1;\n"
        "print arr;\n",
        // var arr 실행 직후 (arr[0] 대입 전) watches 조회
        "watch arr\nstep\nwatches\ncontinue\n");
    EXPECT_TRUE(contains(out, "[WATCH] arr = [nil, nil]"));
}

// runDebug 에서 optimizer 루프가 실행되는 경로 커버 (Shell.cpp line 158)
TEST(DebugModeTest, RunDebug_WithOptimizer_ProducesCorrectOutput) {
    Shell shell;
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    auto path = writeTempScript("debug", "print 2 + 3;\n");
    std::istringstream in("continue\n");
    std::ostringstream out;
    EXPECT_EQ(shell.runDebug(path, in, out), 0);
    EXPECT_TRUE(contains(out.str(), "5"));
}
