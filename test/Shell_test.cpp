#include <gtest/gtest.h>
#include "src/shell/Shell.h"

// ── Shell TDD ─────────────────────────────────────────────────────
// 각 테스트는 Shell 인스턴스를 새로 생성해 독립적으로 실행
// 단, 상태 유지 테스트는 같은 인스턴스를 여러 번 호출

// ════════════════════════════════════════════════════
// Wave 1 — 빈 입력 / 기본 출력
// ════════════════════════════════════════════════════

TEST(ShellTest, EmptyLineProducesNoOutput) {
    Shell shell;
    EXPECT_EQ(shell.runLine(""), "");
}

TEST(ShellTest, PrintIntegerLiteral) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 5;"), "5\n");
}

TEST(ShellTest, PrintWholeFloatAsInteger) {
    // 5.0은 소수점 없이 5로 출력
    Shell shell;
    EXPECT_EQ(shell.runLine("print 5.0;"), "5\n");
}

TEST(ShellTest, PrintDecimalNumber) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 3.14;"), "3.14\n");
}

TEST(ShellTest, PrintStringLiteral) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print \"hello\";"), "hello\n");
}

TEST(ShellTest, PrintBoolTrue) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print true;"), "true\n");
}

TEST(ShellTest, PrintBoolFalse) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print false;"), "false\n");
}

// ════════════════════════════════════════════════════
// Wave 2 — 산술 및 연산자
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

TEST(ShellTest, PrintArithmeticPrecedence) {
    // 1 + 2 * 3 = 1 + 6 = 7
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 + 2 * 3;"), "7\n");
}

TEST(ShellTest, PrintGroupingOverridesPrecedence) {
    // (1 + 2) * 3 = 9
    Shell shell;
    EXPECT_EQ(shell.runLine("print (1 + 2) * 3;"), "9\n");
}

TEST(ShellTest, PrintUnaryMinus) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print -3 + 2;"), "-1\n");
}

TEST(ShellTest, PrintStringConcatenation) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print \"Hello, \" + \"World!\";"), "Hello, World!\n");
}

// ════════════════════════════════════════════════════
// Wave 3 — 비교 / 논리 연산자
// ════════════════════════════════════════════════════

TEST(ShellTest, PrintLessThanTrue) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 < 2;"), "true\n");
}

TEST(ShellTest, PrintGreaterThanFalse) {
    Shell shell;
    EXPECT_EQ(shell.runLine("print 3 > 5;"), "false\n");
}

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
// Wave 4 — 변수 선언 / 대입 / 상태 유지
// ════════════════════════════════════════════════════

TEST(ShellTest, VarDeclAndPrint) {
    Shell shell;
    shell.runLine("var x = 42;");
    EXPECT_EQ(shell.runLine("print x;"), "42\n");
}

TEST(ShellTest, VarReassignment) {
    Shell shell;
    shell.runLine("var a = 10;");
    shell.runLine("a = a + 5;");
    EXPECT_EQ(shell.runLine("print a;"), "15\n");
}

TEST(ShellTest, StatePersistsBetweenLines) {
    Shell shell;
    shell.runLine("var a = 10;");
    shell.runLine("var b = 20;");
    EXPECT_EQ(shell.runLine("print a + b;"), "30\n");
}

TEST(ShellTest, MultipleVarsIndependent) {
    Shell shell;
    shell.runLine("var x = 1;");
    shell.runLine("var y = 2;");
    EXPECT_EQ(shell.runLine("print x;"), "1\n");
    EXPECT_EQ(shell.runLine("print y;"), "2\n");
}

// ════════════════════════════════════════════════════
// Wave 5 — 제어 흐름
// ════════════════════════════════════════════════════

TEST(ShellTest, IfTrue) {
    Shell shell;
    EXPECT_EQ(shell.runLine("if (true) print \"yes\";"), "yes\n");
}

TEST(ShellTest, IfFalseElse) {
    Shell shell;
    EXPECT_EQ(shell.runLine("if (false) print \"no\"; else print \"yes\";"), "yes\n");
}

TEST(ShellTest, ForLoopProducesMultipleLines) {
    Shell shell;
    EXPECT_EQ(shell.runLine("for (var j = 0; j < 3; j = j + 1) { print j; }"),
              "0\n1\n2\n");
}

TEST(ShellTest, BlockScopeInSingleLine) {
    Shell shell;
    shell.runLine("var x = \"global\";");
    shell.runLine("{ var x = \"inner\"; print x; }");
    EXPECT_EQ(shell.runLine("print x;"), "global\n");
}

// ════════════════════════════════════════════════════
// Wave 6 — 에러 처리 (예외가 출력에 포함됨)
// ════════════════════════════════════════════════════

TEST(ShellTest, ParseErrorMissingSemicolon) {
    Shell shell;
    std::string out = shell.runLine("print 1 + 2");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, ParseErrorUnclosedParen) {
    Shell shell;
    std::string out = shell.runLine("print (1 + 2;");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, ParseErrorInvalidAssignmentTarget) {
    Shell shell;
    shell.runLine("var a = 1;");
    shell.runLine("var b = 2;");
    std::string out = shell.runLine("a + b = 3;");
    EXPECT_NE(out.find("Invalid assignment target"), std::string::npos);
}

TEST(ShellTest, ParseErrorBadToken) {
    Shell shell;
    std::string out = shell.runLine("print * 5;");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, CheckerErrorSelfReferenceInInit) {
    Shell shell;
    std::string out = shell.runLine("{ var a = a; }");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, CheckerErrorDuplicateVar) {
    Shell shell;
    std::string out = shell.runLine("{ var a = \"hi\"; var a = 3; }");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, RuntimeErrorUndefinedVariable) {
    Shell shell;
    std::string out = shell.runLine("print notDefined;");
    EXPECT_NE(out.find("notDefined"), std::string::npos);
}

TEST(ShellTest, RuntimeErrorUnaryMinusOnString) {
    Shell shell;
    std::string out = shell.runLine("print -\"FabCoding\";");
    EXPECT_FALSE(out.empty());
}

TEST(ShellTest, ErrorDoesNotCrashShell) {
    // 에러 발생 후에도 Shell이 계속 동작해야 함
    Shell shell;
    shell.runLine("print notDefined;");
    EXPECT_EQ(shell.runLine("print 42;"), "42\n");
}
