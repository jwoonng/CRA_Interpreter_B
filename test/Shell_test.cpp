#include <gtest/gtest.h>
#include "src/shell/Shell.h"
#include <sstream>

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

// ════════════════════════════════════════════════════
// Wave 9 — 멀티라인 입력 (Shell::run 기반)
// Shell::run()에 istringstream 으로 여러 줄을 넘겨
// { } 누적 후 한 번에 실행되는지 검증한다.
// ════════════════════════════════════════════════════

// 출력에서 "> " / "... " 프롬프트를 제거하고 프로그램 출력만 반환한다.
static std::string runScript(const std::string& src) {
    Shell shell;
    std::istringstream in(src);
    std::ostringstream out;
    shell.run(in, out);
    std::string s = out.str();
    std::string result;
    for (size_t i = 0; i < s.size(); ) {
        if (s.compare(i, 4, "... ") == 0) {
            i += 4;
            while (i < s.size() && s[i] == ' ') ++i;  // depth indent
            continue;
        }
        if (s.compare(i, 2, "> ") == 0) { i += 2; continue; }
        result += s[i++];
    }
    return result;
}

// Func 멀티라인 — 선언과 호출 모두 여러 줄
TEST(ShellTest, MultiLine_Func_BasicCall) {
    EXPECT_EQ(runScript(
        "Func add(a, b) {\n"
        "    return a + b;\n"
        "}\n"
        "print add(3, 7);"
    ), "10\n");
}

// Func 멀티라인 — 반환값을 변수에 저장
TEST(ShellTest, MultiLine_Func_ReturnValueStoredInVar) {
    EXPECT_EQ(runScript(
        "Func mul(a, b) {\n"
        "    return a * b;\n"
        "}\n"
        "var r = mul(4, 5);\n"
        "print r;"
    ), "20\n");
}

// for 멀티라인 — 본문이 여러 줄
TEST(ShellTest, MultiLine_For_Accumulator) {
    EXPECT_EQ(runScript(
        "var s = 0;\n"
        "for (var i = 1; i <= 5; i = i + 1) {\n"
        "    s = s + i;\n"
        "}\n"
        "print s;"
    ), "15\n");
}

// 중첩 블록 멀티라인
TEST(ShellTest, MultiLine_NestedBlocks) {
    EXPECT_EQ(runScript(
        "var x = 0;\n"
        "{\n"
        "    var y = 10;\n"
        "    {\n"
        "        x = y + 5;\n"
        "    }\n"
        "}\n"
        "print x;"
    ), "15\n");
}

// 재귀 함수 멀티라인
TEST(ShellTest, MultiLine_Func_Recursive) {
    EXPECT_EQ(runScript(
        "Func fact(n) {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * fact(n - 1);\n"
        "}\n"
        "print fact(5);"
    ), "120\n");
}

// Func + for 조합 멀티라인
TEST(ShellTest, MultiLine_Func_WithForLoop) {
    EXPECT_EQ(runScript(
        "Func sumTo(n) {\n"
        "    var s = 0;\n"
        "    for (var i = 1; i <= n; i = i + 1) {\n"
        "        s = s + i;\n"
        "    }\n"
        "    return s;\n"
        "}\n"
        "print sumTo(10);"
    ), "55\n");
}

// 문자열 리터럴 안의 { } 는 깊이 계산에서 제외되어야 한다
TEST(ShellTest, MultiLine_StringWithBraces_NotCountedAsScope) {
    EXPECT_EQ(runScript(
        "Func f() {\n"
        "    return \"{\";\n"
        "}\n"
        "print f();"
    ), "{\n");
}

// ════════════════════════════════════════════════════
// Wave 10 — 스코프 가시성
// ════════════════════════════════════════════════════

// 내부 블록에서 외부 변수에 접근 가능
TEST(ShellTest, Scope_InnerCanAccessOuterVar) {
    std::string out = runScript(
        "{\n"
        "   var a = 10;\n"
        "   {\n"
        "      print a;\n"
        "      var b = 3;\n"
        "   }\n"
        "   print b;\n"
        "}");
    EXPECT_NE(out.find("10"), std::string::npos);
    EXPECT_NE(out.find("Undefined variable"), std::string::npos);
}

// 외부 블록에서 내부 변수에 접근 불가
TEST(ShellTest, Scope_OuterCannotAccessInnerVar) {
    std::string out = runScript(
        "{\n"
        "   {\n"
        "      var inner = 42;\n"
        "   }\n"
        "   print inner;\n"
        "}"
    );
    EXPECT_NE(out.find("Undefined variable"), std::string::npos);
}

// 블록이 끝나면 내부 변수는 사라진다 (shadow 후 복원)
TEST(ShellTest, Scope_ShadowedVarRestoredAfterBlock) {
    EXPECT_EQ(runScript(
        "var x = 1;\n"
        "{\n"
        "   var x = 2;\n"
        "   print x;\n"
        "}\n"
        "print x;"
    ), "2\n1\n");
}
