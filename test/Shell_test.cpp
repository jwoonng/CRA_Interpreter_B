#include <gtest/gtest.h>
#include "src/shell/Shell.h"

// ── Shell 통합 테스트 ──────────────────────────────────────────────
// Script_test.cpp(E2E)와 중복되지 않는 Shell 고유 동작을 검증한다.
// 각 테스트는 Shell 인스턴스를 새로 생성해 독립적으로 실행한다.

// ════════════════════════════════════════════════════
// Wave 1 — 빈 입력 / 기본 출력
// ════════════════════════════════════════════════════

TEST(ShellTest, EmptyLineProducesNoOutput) {
    Shell shell;
    EXPECT_EQ(shell.runLine(""), "");
}

TEST(ShellTest, PrintStringLiteral) {
    Shell shell;
    EXPECT_EQ(shell.runLine(R"(print "hello";)"), "hello\n");
}

// ════════════════════════════════════════════════════
// Wave 2 — 기본 산술 연산자 (단일 연산)
// ════════════════════════════════════════════════════

TEST(ShellTest, PrintAddition) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 + 2;"), "3\n");
}

TEST(ShellTest, PrintSubtraction) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 10 - 3;"), "7\n");
}

TEST(ShellTest, PrintMultiplication) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 2 * 4;"), "8\n");
}

TEST(ShellTest, PrintDivision) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 9 / 3;"), "3\n");
}

// ════════════════════════════════════════════════════
// Wave 3 — 동등 / 논리 연산자
// ════════════════════════════════════════════════════

TEST(ShellTest, PrintEqualEqual) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 == 1;"), "true\n");
}

TEST(ShellTest, PrintBangEqual) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 != 2;"), "true\n");
}

TEST(ShellTest, PrintLogicalAnd) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print true and false;"), "false\n");
}

TEST(ShellTest, PrintLogicalOr) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print false or true;"), "true\n");
}

// ════════════════════════════════════════════════════
// Wave 4 — 변수 선언 / 상태 유지
// ════════════════════════════════════════════════════

TEST(ShellTest, VarDeclAndPrint) {
    Shell shell;
    shell.runLine("var x = 42;");
    EXPECT_EQ(shell.runLine("print x;"), "42\n");
}

TEST(ShellTest, MultipleVarsIndependent) {
    Shell shell;
    shell.runLine("var x = 1;");
    shell.runLine("var y = 2;");
    EXPECT_EQ(shell.runLine("print x;"), "1\n");
    EXPECT_EQ(shell.runLine("print y;"), "2\n");
}

// ════════════════════════════════════════════════════
// Wave 5 — 런타임 에러 / 에러 복원
// ════════════════════════════════════════════════════

TEST(ShellTest, RuntimeErrorDivisionByZero) {
    Shell shell;
    std::string out = shell.runLine("print 1 / 0;");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, ErrorDoesNotCrashShell) {
    Shell shell;
    shell.runLine("print notDefined;");
    EXPECT_EQ(shell.runLine("print 42;"), "42\n");
}

// ════════════════════════════════════════════════════
// Wave 6 — 함수 멀티라인 (REPL 줄 간 상태 유지)
// ════════════════════════════════════════════════════

TEST(ShellTest, Func_DefinedAndCalledOnSeparateLines) {
    Shell shell;
    shell.runLine("Func add(a, b) { return a + b; }");
    EXPECT_EQ(shell.runLine("print add(3, 7);"), "10\n");
}

TEST(ShellTest, Func_ReturnValueStoredInVar_AcrossLines) {
    Shell shell;
    shell.runLine("Func add(a, b) { return a + b; }");
    shell.runLine("var ret = add(3, 7);");
    EXPECT_EQ(shell.runLine("print ret;"), "10\n");
}

TEST(ShellTest, Func_RecursiveFactorial_AcrossLines) {
    Shell shell;
    shell.runLine("Func fact(n) { if (n <= 1) return 1; return n * fact(n - 1); }");
    EXPECT_EQ(shell.runLine("print fact(5);"), "120\n");
}

TEST(ShellTest, Func_CallingAnotherFunc_AcrossLines) {
    Shell shell;
    shell.runLine("Func dbl(x) { return x * 2; }");
    shell.runLine("Func quad(x) { return dbl(dbl(x)); }");
    EXPECT_EQ(shell.runLine("print quad(3);"), "12\n");
}

TEST(ShellTest, Func_AccessesGlobalVar) {
    Shell shell;
    shell.runLine("var x = 10;");
    shell.runLine("Func getX() { return x; }");
    EXPECT_EQ(shell.runLine("print getX();"), "10\n");
}

TEST(ShellTest, Func_ModifiesGlobalVar) {
    Shell shell;
    shell.runLine("var counter = 0;");
    shell.runLine("Func inc() { counter = counter + 1; }");
    shell.runLine("inc();");
    EXPECT_EQ(shell.runLine("print counter;"), "1\n");
}

// ════════════════════════════════════════════════════
// Wave 7 — Checker 상태 복원
// ════════════════════════════════════════════════════

TEST(ShellTest, CheckerState_GhostScopeCleared_AfterBlockError) {
    Shell shell;
    shell.runLine("{ var a = a; }");
    EXPECT_EQ(shell.runLine("var a = 10;"), "");
    EXPECT_EQ(shell.runLine("print a;"), "10\n");
}

TEST(ShellTest, CheckerState_FunctionDepthResetAfterBodyError) {
    Shell shell;
    shell.runLine("Func foo() { var x = x; }");
    std::string out = shell.runLine("return 5;");
    EXPECT_NE(out.find("function"), std::string::npos);
}

// ════════════════════════════════════════════════════
// Wave 8 — 배열 멀티라인 (REPL 줄 간 상태 유지)
// ════════════════════════════════════════════════════

TEST(ShellTest, Array_PersistsAndWritable_AcrossLines) {
    Shell shell;
    shell.runLine("var arr = Array(3);");
    shell.runLine("arr[0] = 42;");
    EXPECT_EQ(shell.runLine("print arr[0];"), "42\n");
}

TEST(ShellTest, Array_MultipleElements_AcrossLines) {
    Shell shell;
    shell.runLine("var arr = Array(3);");
    shell.runLine("arr[0] = 10;");
    shell.runLine("arr[1] = 20;");
    shell.runLine("arr[2] = 30;");
    EXPECT_EQ(shell.runLine("print arr[0] + arr[1] + arr[2];"), "60\n");
}
