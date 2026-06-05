#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include "src/shell/Debugger.h"
#include "src/shell/ShellLauncher.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
#include "src/optimizer/StaticBindingOptimizer.h"
#include <filesystem>
#include <fstream>
#include <sstream>

// ── 공장 제어 쉘 테스트 ────────────────────────────────────────────
// Ch.5 요구사항 검증: 프롬프트 모드(REPL) / 파일 모드 / 디버그 모드

namespace {

// 부분 문자열 등장 횟수
int countOf(const std::string& haystack, const std::string& needle) {
    int n = 0;
    for (auto pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + needle.size()))
        ++n;
    return n;
}

// 디버거 실행 헬퍼: (출력, 종료 코드)
std::pair<std::string, int> debugRun(const std::string& source,
                                     const std::string& commands) {
    Debugger dbg;
    std::istringstream cmd(commands);
    std::ostringstream out;
    int rc = dbg.runSource(source, cmd, out);
    return {out.str(), rc};
}

// 임시 스크립트 파일 RAII
struct TempScript {
    std::filesystem::path path;
    explicit TempScript(const std::string& name, const std::string& content) {
        path = std::filesystem::temp_directory_path() / name;
        std::ofstream f(path);
        f << content;
    }
    ~TempScript() { std::error_code ec; std::filesystem::remove(path, ec); }
};

} // namespace

// ════════════════════════════════════════════════════
// Wave 1 — 프롬프트 모드(REPL): exit / quit 종료
// ════════════════════════════════════════════════════

TEST(ReplModeTest, ExitTerminatesSession) {
    Shell shell;
    std::istringstream in("print 1;\nexit\nprint 2;\n");
    std::ostringstream out;
    shell.run(in, out);
    EXPECT_NE(out.str().find("1\n"), std::string::npos);
    EXPECT_EQ(out.str().find("2\n"), std::string::npos);   // exit 이후 실행 안 됨
}

TEST(ReplModeTest, QuitTerminatesSession) {
    Shell shell;
    std::istringstream in("var x = 5;\nquit\nprint x;\n");
    std::ostringstream out;
    shell.run(in, out);
    EXPECT_EQ(out.str().find("5\n"), std::string::npos);
}

TEST(ReplModeTest, ExitWithSurroundingWhitespace) {
    Shell shell;
    std::istringstream in("  exit  \nprint 1;\n");
    std::ostringstream out;
    shell.run(in, out);
    EXPECT_EQ(out.str().find("1\n"), std::string::npos);
}

TEST(ReplModeTest, GlobalStatePersistsUntilExit) {
    Shell shell;
    std::istringstream in("var x = 40;\nx = x + 2;\nprint x;\nexit\n");
    std::ostringstream out;
    shell.run(in, out);
    EXPECT_NE(out.str().find("42\n"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 2 — 파일 모드: 전체 소스 실행 / 오류 처리
// ════════════════════════════════════════════════════

TEST(FileModeTest, RunSourceExecutesWholeProgram) {
    Shell shell;
    std::ostringstream out;
    int rc = shell.runSource(
        "var total = 0;\n"
        "for (var i = 1; i <= 4; i = i + 1) {\n"
        "    total = total + i;\n"
        "}\n"
        "print total;\n", out);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.str(), "10\n");
}

TEST(FileModeTest, RunSourceSupportsFunctionsAndArrays) {
    Shell shell;
    std::ostringstream out;
    int rc = shell.runSource(
        "Func square(n) { return n * n; }\n"
        "var arr = Array(3);\n"
        "arr[0] = square(2);\n"
        "arr[1] = square(3);\n"
        "arr[2] = arr[0] + arr[1];\n"
        "print arr[2];\n", out);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.str(), "13\n");
}

TEST(FileModeTest, RuntimeErrorPrintsLineAndStopsImmediately) {
    Shell shell;
    std::ostringstream out;
    int rc = shell.runSource(
        "print 1;\n"
        "print 1 / 0;\n"
        "print 2;\n", out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.str().find("1\n"), std::string::npos);
    EXPECT_NE(out.str().find("[line 2]"), std::string::npos);  // 오류 줄 번호
    EXPECT_EQ(out.str().find("2\n"), std::string::npos);       // 즉시 종료
}

TEST(FileModeTest, MissingFilePrintsClearError) {
    Shell shell;
    std::ostringstream out;
    int rc = shell.runFile("no_such_file_xyz.cf", out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.str().find("cannot open file"), std::string::npos);
    EXPECT_NE(out.str().find("no_such_file_xyz.cf"), std::string::npos);
}

TEST(FileModeTest, RunFileExecutesScriptOnDisk) {
    TempScript script("codefab_filemode_test.cf",
        "Func fact(n) { if (n <= 1) return 1; return n * fact(n - 1); }\n"
        "print fact(5);\n");
    Shell shell;
    std::ostringstream out;
    int rc = shell.runFile(script.path.string(), out);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.str(), "120\n");
}

TEST(FileModeTest, RunSourceWorksWithOptimizers) {
    // 파일 모드는 최적화 체인(ConstantFolding + StaticBinding)과 함께 사용된다
    Shell shell;
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    std::ostringstream out;
    int rc = shell.runSource(
        "var total = 0;\n"
        "{\n"
        "    var inner = 1 + 2 * 3;\n"
        "    total = total + inner;\n"
        "}\n"
        "print total;\n", out);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.str(), "7\n");
}

// ════════════════════════════════════════════════════
// Wave 3 — 디버그 모드: stepping (step / next / continue)
// ════════════════════════════════════════════════════

TEST(DebugModeTest, StepStopsAtEveryStatement) {
    auto [out, rc] = debugRun(
        "var x = 1;\n"
        "var y = 2;\n"
        "print x + y;\n",
        "step\nstep\nstep\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("stopped at line 1: var x = 1;"), std::string::npos);
    EXPECT_NE(out.find("stopped at line 2: var y = 2;"), std::string::npos);
    EXPECT_NE(out.find("stopped at line 3: print x + y;"), std::string::npos);
    EXPECT_NE(out.find("3\n"), std::string::npos);              // print 실행 결과
    EXPECT_NE(out.find("program finished"), std::string::npos);
}

TEST(DebugModeTest, StepEntersBlock) {
    auto [out, rc] = debugRun(
        "var a = 0;\n"
        "{\n"
        "    var b = 1;\n"
        "    a = b;\n"
        "}\n"
        "print a;\n",
        "step\nstep\nstep\nstep\nstep\nstep\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("stopped at line 3"), std::string::npos);  // 블록 내부 진입
    EXPECT_NE(out.find("stopped at line 4"), std::string::npos);
}

TEST(DebugModeTest, NextSkipsBlockInterior) {
    auto [out, rc] = debugRun(
        "var a = 0;\n"
        "{\n"
        "    var b = 1;\n"
        "    a = b;\n"
        "}\n"
        "print a;\n",
        "step\nnext\nnext\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("stopped at line 2"), std::string::npos);   // 블록 자체에는 정지
    EXPECT_EQ(out.find("stopped at line 3"), std::string::npos);   // 내부 진입 X
    EXPECT_EQ(out.find("stopped at line 4"), std::string::npos);
    EXPECT_NE(out.find("stopped at line 6"), std::string::npos);   // 블록 다음 Stmt
    EXPECT_NE(out.find("1\n"), std::string::npos);
}

TEST(DebugModeTest, StepEntersFunctionBody) {
    auto [out, rc] = debugRun(
        "Func add(a, b) {\n"
        "    return a + b;\n"
        "}\n"
        "print add(1, 2);\n",
        "step\nstep\nstep\nstep\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("stopped at line 4"), std::string::npos);  // 호출 지점
    EXPECT_NE(out.find("stopped at line 2: return a + b;"), std::string::npos);  // 함수 내부
    EXPECT_NE(out.find("3\n"), std::string::npos);
}

TEST(DebugModeTest, NextSkipsFunctionBody) {
    auto [out, rc] = debugRun(
        "Func add(a, b) {\n"
        "    return a + b;\n"
        "}\n"
        "print add(1, 2);\n",
        "next\nnext\nnext\n");
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.find("stopped at line 2"), std::string::npos);  // 함수 내부 진입 X
    EXPECT_NE(out.find("3\n"), std::string::npos);
}

TEST(DebugModeTest, ContinueRunsToEndWithoutBreakpoints) {
    auto [out, rc] = debugRun(
        "print 1;\n"
        "print 2;\n"
        "print 3;\n",
        "continue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(countOf(out, "stopped at line"), 1);  // 최초 1회만 정지
    EXPECT_NE(out.find("1\n2\n3\n"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 4 — 디버그 모드: breakpoint (break / breakpoints / remove)
// ════════════════════════════════════════════════════

TEST(DebugModeTest, BreakpointStopsAtLine) {
    auto [out, rc] = debugRun(
        "var i = 0;\n"
        "i = 1;\n"
        "i = 2;\n"
        "print i;\n",
        "break 3\ncontinue\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("breakpoint set at line 3"), std::string::npos);
    EXPECT_NE(out.find("stopped at line 3: i = 2;"), std::string::npos);
    EXPECT_EQ(out.find("stopped at line 2"), std::string::npos);  // bp 없는 줄 통과
    EXPECT_NE(out.find("2\n"), std::string::npos);
}

TEST(DebugModeTest, BreakpointInLoopHitsEveryIteration) {
    auto [out, rc] = debugRun(
        "var s = 0;\n"
        "for (var i = 0; i < 2; i = i + 1) {\n"
        "    s = s + 1;\n"
        "}\n"
        "print s;\n",
        "break 3\ncontinue\ncontinue\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(countOf(out, "stopped at line 3"), 2);  // 반복마다 정지
    EXPECT_NE(out.find("2\n"), std::string::npos);
}

TEST(DebugModeTest, BreakpointsListsAndRemoveDeletes) {
    auto [out, rc] = debugRun(
        "var i = 0;\n"
        "i = 1;\n"
        "i = 2;\n"
        "print i;\n",
        "break 2\nbreak 3\nbreakpoints\nremove 2\nbreakpoints\ncontinue\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("breakpoints: 2 3"), std::string::npos);
    EXPECT_NE(out.find("breakpoint removed at line 2"), std::string::npos);
    EXPECT_NE(out.find("breakpoints: 3"), std::string::npos);
    EXPECT_EQ(out.find("stopped at line 2"), std::string::npos);  // 제거된 bp 통과
    EXPECT_NE(out.find("stopped at line 3"), std::string::npos);
}

TEST(DebugModeTest, RemoveNonexistentBreakpointReportsIt) {
    auto [out, rc] = debugRun("print 1;\n", "remove 7\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("no breakpoint at line 7"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 5 — 디버그 모드: watch / unwatch / watches / inspect
// ════════════════════════════════════════════════════

TEST(DebugModeTest, WatchAutoPrintsAtEveryPause) {
    auto [out, rc] = debugRun(
        "var x = 10;\n"
        "x = 20;\n"
        "print x;\n",
        "watch x\nstep\nstep\nstep\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("watching 'x'"), std::string::npos);
    EXPECT_NE(out.find("watch x = 10"), std::string::npos);  // line 2 정지 시
    EXPECT_NE(out.find("watch x = 20"), std::string::npos);  // line 3 정지 시
}

TEST(DebugModeTest, WatchUndefinedVariableShowsNotDefined) {
    auto [out, rc] = debugRun(
        "var x = 1;\n"
        "print x;\n",
        "watch zzz\nstep\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("watch zzz = <not defined>"), std::string::npos);
}

TEST(DebugModeTest, UnwatchStopsAutoPrint) {
    auto [out, rc] = debugRun(
        "var x = 1;\n"
        "x = 2;\n"
        "print x;\n",
        "watch x\nstep\nunwatch x\nstep\nstep\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("unwatched 'x'"), std::string::npos);
    // line 2 정지 시 1회 출력 후, unwatch 이후 더 이상 출력 없음
    EXPECT_EQ(countOf(out, "watch x = "), 1);
}

TEST(DebugModeTest, WatchesShowsNearestScopeValue) {
    auto [out, rc] = debugRun(
        "var x = 1;\n"
        "{\n"
        "    var x = 99;\n"
        "    print x;\n"
        "}\n",
        "step\nstep\nstep\nwatches\nwatch x\nwatches\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("no watches"), std::string::npos);
    EXPECT_NE(out.find("watch x = 99"), std::string::npos);  // 가장 인접한 스코프 값
}

TEST(DebugModeTest, InspectShowsCurrentScopeVariables) {
    auto [out, rc] = debugRun(
        "var g = 7;\n"
        "{\n"
        "    var a = 1;\n"
        "    var b = 2;\n"
        "    print a + b;\n"
        "}\n",
        "step\nstep\nstep\nstep\ninspect\ncontinue\n");
    EXPECT_EQ(rc, 0);
    // print 직전: 현재 스코프(블록)에는 a, b 만 존재
    EXPECT_NE(out.find("a = 1"), std::string::npos);
    EXPECT_NE(out.find("b = 2"), std::string::npos);
}

TEST(DebugModeTest, InspectInsideFunctionShowsParameters) {
    auto [out, rc] = debugRun(
        "Func add(a, b) {\n"
        "    return a + b;\n"
        "}\n"
        "print add(3, 4);\n",
        // 정지 순서: line1(Func 선언) → line4(호출) → line2(함수 내부)
        "step\nstep\ninspect\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("a = 3"), std::string::npos);
    EXPECT_NE(out.find("b = 4"), std::string::npos);
}

TEST(DebugModeTest, InspectEmptyScope) {
    auto [out, rc] = debugRun("print 1;\n", "inspect\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("(no variables in current scope)"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 6 — 디버그 모드: 종료 / 오류 / 잘못된 명령
// ════════════════════════════════════════════════════

TEST(DebugModeTest, QuitAbortsExecution) {
    auto [out, rc] = debugRun(
        "print 1;\n"
        "print 2;\n",
        "quit\n");
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out.find("1\n"), std::string::npos);  // 아무것도 실행되지 않음
    EXPECT_NE(out.find("session terminated"), std::string::npos);
}

TEST(DebugModeTest, CommandEofTerminatesSession) {
    auto [out, rc] = debugRun("print 1;\nprint 2;\n", "");  // 명령 입력 즉시 EOF
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("session terminated"), std::string::npos);
}

TEST(DebugModeTest, RuntimeErrorReportsLine) {
    auto [out, rc] = debugRun(
        "var x = 1;\n"
        "print x / 0;\n",
        "continue\n");
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.find("[line 2]"), std::string::npos);
}

TEST(DebugModeTest, UnknownCommandShowsHelp) {
    auto [out, rc] = debugRun("print 1;\n", "frobnicate\ncontinue\n");
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("unknown command: frobnicate"), std::string::npos);
    EXPECT_NE(out.find("1\n"), std::string::npos);  // 잘못된 명령 후에도 계속 진행 가능
}

TEST(DebugModeTest, MissingFilePrintsClearError) {
    Debugger dbg;
    std::istringstream cmd("");
    std::ostringstream out;
    int rc = dbg.runFile("no_such_debug_target.cf", cmd, out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.str().find("cannot open file"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 7 — Executor IStmtHook (Test Double 검증)
// ════════════════════════════════════════════════════

namespace {
// Test Double: Stmt 실행 순서·깊이를 기록하는 Spy
struct RecordingHook : IStmtHook {
    std::vector<std::pair<int, int>> events;  // (line, depth)
    void onBeforeStmt(const Stmt& s, int depth, Environment&) override {
        events.emplace_back(s.line, depth);
    }
};

std::vector<std::pair<int, int>> recordExecution(const std::string& source) {
    Tokenizer tokenizer;
    Parser    parser;
    Checker   checker;
    Executor  executor;
    RecordingHook hook;

    auto tokens = tokenizer.tokenize(source);
    auto stmts  = parser.parse(tokens);
    checker.check(stmts);

    executor.setStmtHook(&hook);
    std::ostringstream out;
    executor.execute(std::move(stmts), out);
    return hook.events;
}
} // namespace

TEST(StmtHookTest, HookReceivesStatementsInExecutionOrder) {
    auto events = recordExecution(
        "var x = 1;\n"
        "print x;\n");
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0], std::make_pair(1, 0));
    EXPECT_EQ(events[1], std::make_pair(2, 0));
}

TEST(StmtHookTest, HookReportsNestedDepthInsideBlock) {
    auto events = recordExecution(
        "var x = 1;\n"
        "{\n"
        "    print x;\n"
        "}\n");
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0], std::make_pair(1, 0));   // var x      (top-level)
    EXPECT_EQ(events[1], std::make_pair(2, 0));   // block      (top-level)
    EXPECT_EQ(events[2], std::make_pair(3, 1));   // print x    (블록 내부)
}

TEST(StmtHookTest, NoHookMeansNoOverhead) {
    // 훅 미등록 시 기존 동작 그대로 (회귀 방지)
    Executor executor;
    Tokenizer tokenizer;
    Parser parser;
    auto stmts = parser.parse(tokenizer.tokenize("print 42;"));
    std::ostringstream out;
    executor.execute(std::move(stmts), out);
    EXPECT_EQ(out.str(), "42\n");
}

// ════════════════════════════════════════════════════
// Wave 8 — launchShell: 모드 선택 디스패치
// ════════════════════════════════════════════════════

namespace {
int launchWith(std::vector<std::string> argStrs,
               const std::string& input, std::string& output) {
    std::vector<char*> argv;
    argStrs.insert(argStrs.begin(), "codefab");  // 프로그램 이름
    for (auto& s : argStrs) argv.push_back(s.data());

    std::istringstream in(input);
    std::ostringstream out;
    int rc = launchShell(static_cast<int>(argv.size()), argv.data(), in, out);
    output = out.str();
    return rc;
}
} // namespace

TEST(ShellLauncherTest, NoArgsStartsRepl) {
    std::string out;
    int rc = launchWith({}, "print 1 + 2;\nexit\n", out);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("3\n"), std::string::npos);
    EXPECT_NE(out.find("> "), std::string::npos);  // 프롬프트 표시
}

TEST(ShellLauncherTest, FileArgRunsFileMode) {
    TempScript script("codefab_launcher_test.cf", "print 7 * 6;\n");
    std::string out;
    int rc = launchWith({script.path.string()}, "", out);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(out, "42\n");
}

TEST(ShellLauncherTest, DebugFlagRunsDebugMode) {
    TempScript script("codefab_launcher_debug_test.cf", "print 99;\n");
    std::string out;
    int rc = launchWith({"--debug", script.path.string()}, "continue\n", out);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(out.find("stopped at line 1"), std::string::npos);
    EXPECT_NE(out.find("99\n"), std::string::npos);
}

TEST(ShellLauncherTest, DebugFlagWithoutFileFails) {
    std::string out;
    int rc = launchWith({"--debug"}, "", out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.find("--debug requires a script file"), std::string::npos);
}

TEST(ShellLauncherTest, UnknownOptionFailsWithUsage) {
    std::string out;
    int rc = launchWith({"--bogus"}, "", out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.find("unknown option"), std::string::npos);
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

TEST(ShellLauncherTest, MissingScriptFileFails) {
    std::string out;
    int rc = launchWith({"definitely_missing.cf"}, "", out);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(out.find("cannot open file"), std::string::npos);
}
