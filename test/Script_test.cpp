#include <gtest/gtest.h>
#include "src/shell/Shell.h"

// ── Gist 스크립트 통합 테스트 ─────────────────────────────────────
// https://gist.github.com/aijeonghwan-star/d1535e870aeb6a4a928142d4d57c191e
//
// 실제 Tokenizer / Parser / Checker / Executor 를 사용하는 End-to-End 테스트
// 각 테스트는 Shell 인스턴스를 통해 스크립트 라인을 순서대로 실행

// ════════════════════════════════════════════════════
// 1. 정상동작 테스트 — 표현식 / 연산자 / 우선순위
// ════════════════════════════════════════════════════

TEST(ScriptTest, ArithmeticPrecedenceMulBeforeAdd) {
    // print 1 + 2 * 3;  expect: 7
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 + 2 * 3;"), "7\n");
}

TEST(ScriptTest, GroupingOverridesPrecedence) {
    // print (1 + 2) * 3;  expect: 9
    Shell shell;
    EXPECT_EQ(shell.runLine("print (1 + 2) * 3;"), "9\n");
}

TEST(ScriptTest, SubtractionLeftAssociative) {
    // print 10 - 4 - 3;  expect: 3
    Shell shell;
    EXPECT_EQ(shell.runLine("print 10 - 4 - 3;"), "3\n");
}

TEST(ScriptTest, DivisionLeftAssociative) {
    // print 8 / 2 / 2;  expect: 2
    Shell shell;
    EXPECT_EQ(shell.runLine("print 8 / 2 / 2;"), "2\n");
}

TEST(ScriptTest, UnaryMinusWithAddition) {
    // print -3 + 2;  expect: -1
    Shell shell;
    EXPECT_EQ(shell.runLine("print -3 + 2;"), "-1\n");
}

TEST(ScriptTest, ComparisonLessThanTrue) {
    // print 1 < 2;  expect: true
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 < 2;"), "true\n");
}

TEST(ScriptTest, ComparisonGreaterEqualTrue) {
    // print 2 >= 2;  expect: true
    Shell shell;
    EXPECT_EQ(shell.runLine("print 2 >= 2;"), "true\n");
}

TEST(ScriptTest, ComparisonLessEqualTrue) {
    // print 1 <= 2;  expect: true
    Shell shell;
    EXPECT_EQ(shell.runLine("print 1 <= 2;"), "true\n");
}

TEST(ScriptTest, ComparisonGreaterThanFalse) {
    // print 3 > 5;  expect: false
    Shell shell;
    EXPECT_EQ(shell.runLine("print 3 > 5;"), "false\n");
}

TEST(ScriptTest, StringConcatenation) {
    // print "Hello, " + "CodeFab!";  expect: Hello, CodeFab!
    Shell shell;
    std::string r = shell.runLine(R"(print "Hello, " + "CodeFab!";)");
    EXPECT_EQ(r, "Hello, CodeFab!\n");
}

TEST(ScriptTest, NumberFormatIntegerNoDecimal) {
    // print 5;  expect: 5
    Shell shell;
    EXPECT_EQ(shell.runLine("print 5;"), "5\n");
}

TEST(ScriptTest, NumberFormatWholeFloatNoDecimal) {
    // print 5.0;  expect: 5  (정수는 .0 없이 출력)
    Shell shell;
    EXPECT_EQ(shell.runLine("print 5.0;"), "5\n");
}

TEST(ScriptTest, NumberFormatDecimalPreserved) {
    // print 3.14;  expect: 3.14
    Shell shell;
    EXPECT_EQ(shell.runLine("print 3.14;"), "3.14\n");
}

TEST(ScriptTest, BoolLiteralTrue) {
    // print true;  expect: true
    Shell shell;
    EXPECT_EQ(shell.runLine("print true;"), "true\n");
}

TEST(ScriptTest, BoolLiteralFalse) {
    // print false;  expect: false
    Shell shell;
    EXPECT_EQ(shell.runLine("print false;"), "false\n");
}

// ════════════════════════════════════════════════════
// 1. 정상동작 테스트 — 변수, 할당, 스코프
// ════════════════════════════════════════════════════

TEST(ScriptTest, VarDeclareAndUse) {
    // var a = 10; var b = 20; print a + b;  expect: 30
    Shell shell;
    shell.runLine("var a = 10;");
    shell.runLine("var b = 20;");
    EXPECT_EQ(shell.runLine("print a + b;"), "30\n");
}

TEST(ScriptTest, VarReassignment) {
    // a = a + 5; print a;  expect: 15
    Shell shell;
    shell.runLine("var a = 10;");
    shell.runLine("a = a + 5;");
    EXPECT_EQ(shell.runLine("print a;"), "15\n");
}

TEST(ScriptTest, BlockScopeVariableShadowing) {
    // var x = "global"; { var x = "inner"; print x; } print x;
    // expect: inner, global
    Shell shell;
    shell.runLine("var x = \"global\";");
    std::string innerOut = shell.runLine("{ var x = \"inner\"; print x; }");
    std::string outerOut = shell.runLine("print x;");
    EXPECT_EQ(innerOut, "inner\n");
    EXPECT_EQ(outerOut, "global\n");
}

TEST(ScriptTest, BlockScopeModifyOuterVar) {
    // var count = 0; { count = count + 1; } print count;  expect: 1
    Shell shell;
    shell.runLine("var count = 0;");
    shell.runLine("{ count = count + 1; }");
    EXPECT_EQ(shell.runLine("print count;"), "1\n");
}

TEST(ScriptTest, NestedScopeAccessOuterVars) {
    // var outer = "A"; { var inner = "B"; { print outer + inner; } }  expect: AB
    Shell shell;
    shell.runLine("var outer = \"A\";");
    std::string result = shell.runLine("{ var inner = \"B\"; { print outer + inner; } }");
    EXPECT_EQ(result, "AB\n");
}

// ════════════════════════════════════════════════════
// 1. 정상동작 테스트 — 제어 흐름
// ════════════════════════════════════════════════════

TEST(ScriptTest, IfTrue) {
    // if (true) print "bbq";  expect: bbq
    Shell shell;
    std::string r = shell.runLine(R"(if (true) print "bbq";)");
    EXPECT_EQ(r, "bbq\n");
}

TEST(ScriptTest, IfFalseElse) {
    // if (false) print "no"; else print "kfc";  expect: kfc
    Shell shell;
    std::string r = shell.runLine(R"(if (false) print "no"; else print "kfc";)");
    EXPECT_EQ(r, "kfc\n");
}

TEST(ScriptTest, DanglingElseBoundToInnerIf) {
    // if (true) if (false) print "kfc"; else print "bbq";  expect: bbq
    Shell shell;
    std::string r = shell.runLine(R"(if (true) if (false) print "kfc"; else print "bbq";)");
    EXPECT_EQ(r, "bbq\n");
}

TEST(ScriptTest, ForLoopZeroToTwo) {
    // for (var j = 0; j < 3; j = j + 1) { print j; }  expect: 0\n1\n2\n
    Shell shell;
    std::string result = shell.runLine("for (var j = 0; j < 3; j = j + 1) { print j; }");
    EXPECT_EQ(result, "0\n1\n2\n");
}

// ════════════════════════════════════════════════════
// 2. 에러 검출 테스트 — 구문 에러 (Parser)
// ════════════════════════════════════════════════════

TEST(ScriptTest, ParseErrorMissingSemicolon) {
    // print 1 + 2  →  에러 출력 (세미콜론 누락)
    Shell shell;
    std::string out = shell.runLine("print 1 + 2");
    EXPECT_FALSE(out.empty());
}

TEST(ScriptTest, ParseErrorUnclosedParen) {
    // print (1 + 2;  →  에러 출력 (닫는 괄호 누락)
    Shell shell;
    std::string out = shell.runLine("print (1 + 2;");
    EXPECT_FALSE(out.empty());
}

TEST(ScriptTest, ParseErrorInvalidAssignmentTarget) {
    // a + b = 3;  →  "Invalid assignment target." 류의 메시지
    Shell shell;
    shell.runLine("var a = 1;");
    shell.runLine("var b = 2;");
    std::string out = shell.runLine("a + b = 3;");
    EXPECT_NE(out.find("Invalid assignment target"), std::string::npos);
}

TEST(ScriptTest, ParseErrorBadTokenInExpression) {
    // print * 5;  →  에러 출력 (표현식 자리에 * 토큰)
    Shell shell;
    std::string out = shell.runLine("print * 5;");
    EXPECT_FALSE(out.empty());
}

// ════════════════════════════════════════════════════
// 2. 에러 검출 테스트 — 정적 에러 (Checker)
// ════════════════════════════════════════════════════

TEST(ScriptTest, CheckerErrorSelfReferenceInInitializer) {
    // { var a = a; }  →  선언 전 사용 오류
    Shell shell;
    std::string out = shell.runLine("{ var a = a; }");
    EXPECT_FALSE(out.empty());
}

TEST(ScriptTest, CheckerErrorDuplicateVarInSameScope) {
    // { var a = "hi"; var a = 3; }  →  중복 선언 오류
    Shell shell;
    std::string out = shell.runLine("{ var a = \"hi\"; var a = 3; }");
    EXPECT_FALSE(out.empty());
}

// ════════════════════════════════════════════════════
// 2. 에러 검출 테스트 — 런타임 에러 (Executor)
// ════════════════════════════════════════════════════

TEST(ScriptTest, RuntimeErrorUndefinedVariable) {
    // print notDefined;  →  "Undefined variable 'notDefined'." 류의 메시지
    Shell shell;
    std::string out = shell.runLine("print notDefined;");
    EXPECT_NE(out.find("notDefined"), std::string::npos);
}

TEST(ScriptTest, RuntimeErrorTypeMismatchPlusNumberString) {
    // print 1 + "HI";  →  타입 불일치 에러
    Shell shell;
    std::string out = shell.runLine("print 1 + \"HI\";");
    EXPECT_FALSE(out.empty());
}

TEST(ScriptTest, RuntimeErrorUnaryMinusOnString) {
    // print -"FabCoding";  →  "Operand must be a number." 류의 메시지
    Shell shell;
    std::string out = shell.runLine("print -\"FabCoding\";");
    EXPECT_NE(out.find("number"), std::string::npos);
}

// ════════════════════════════════════════════════════
// 3. 에러 복원 — 에러 후 Shell 지속 동작 확인
// ════════════════════════════════════════════════════

TEST(ScriptTest, ShellContinuesAfterParseError) {
    Shell shell;
    shell.runLine("print 1 + 2");        // 에러 (세미콜론 누락)
    EXPECT_EQ(shell.runLine("print 99;"), "99\n");
}

TEST(ScriptTest, ShellContinuesAfterRuntimeError) {
    Shell shell;
    shell.runLine("print notDefined;");  // 런타임 에러
    EXPECT_EQ(shell.runLine("print 1;"), "1\n");
}

TEST(ScriptTest, VarStatePersistsAfterError) {
    Shell shell;
    shell.runLine("var x = 100;");
    shell.runLine("print notDefined;");  // 에러
    EXPECT_EQ(shell.runLine("print x;"), "100\n");  // x 는 살아있어야 함
}
