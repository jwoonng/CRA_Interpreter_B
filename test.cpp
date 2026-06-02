/**
 * test.cpp - CodeFab Interpreter Unit Tests (Google Test / GMock)
 *
 * 컴파일 예시:
 *   g++ -std=c++17 test.cpp -lgtest -lgtest_main -lgmock -pthread -o test
 *
 * 헤더 경로는 팀 구현에 맞게 추가하세요.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// TODO: 팀 구현 헤더 include
// #include "Tokenizer.h"
// #include "Parser.h"
// #include "Checker.h"
// #include "Executor.h"
// #include "LangFactory.h"

// ═════════════════════════════════════════════════════════════
//  Assembler Unit - Tokenizer
// ═════════════════════════════════════════════════════════════

TEST(TokenizerTest, VarDeclarationTokens) {
    // "var a = 3;" 를 Tokenizer에 입력한다
    // 생성된 토큰 목록이 아래 순서·타입임을 확인한다
    //   tokens[0].type == VAR_DECL
    //   tokens[1].type == IDENTIFIER,  tokens[1].lexeme == "a"
    //   tokens[2].type == EQUAL
    //   tokens[3].type == NUMBER,      tokens[3].value  == 3.0
    //   tokens[4].type == SEMICOLON
    //   tokens[5].type == EOF_TOKEN
}

TEST(TokenizerTest, IfExpressionTokens) {
    // "if ( x > 10 )" 를 Tokenizer에 입력한다
    // 생성된 토큰이 아래 순서임을 확인한다
    //   IF / LEFT_PAREN / IDENTIFIER("x") / GREATER / NUMBER(10) / RIGHT_PAREN / EOF
}

TEST(TokenizerTest, ArithmeticOperatorTokens) {
    // "a + b * 3" 을 Tokenizer에 입력한다
    // 생성된 토큰이 아래 순서임을 확인한다
    //   IDENTIFIER("a") / PLUS / IDENTIFIER("b") / STAR / NUMBER(3) / EOF
}

TEST(TokenizerTest, StringAndBoolLiteralTokens) {
    // "\"hello\"" → STRING 토큰, lexeme == "hello" 확인
    // "true"      → TRUE  토큰 확인
    // "false"     → FALSE 토큰 확인
}

TEST(TokenizerTest, NumberLiteralFormat) {
    // "3.14" → NUMBER 토큰, value == 3.14 확인
    // "5"    → NUMBER 토큰, value == 5.0  확인
}

TEST(TokenizerTest, KeywordRecognition) {
    // "print" / "var" / "if" / "else" / "for" / "and" / "or"
    // 각각 올바른 키워드 TokenType으로 인식되는지 확인
    // (IDENTIFIER 가 아니라 예약어 타입이어야 한다)
}

TEST(TokenizerTest, WhitespaceAndNewlineIgnored) {
    // "  var   a  =  3  ;  " (공백 다수)
    // "\nvar\na\n=\n3\n;\n"  (줄바꿈)
    // 두 입력 모두 동일한 토큰 목록을 생성하는지 확인
}

// ═════════════════════════════════════════════════════════════
//  Assembler Unit - Parser (문법 Tree 생성)
// ═════════════════════════════════════════════════════════════

TEST(ParserTest, LiteralExpression) {
    // "123;" 을 파싱한다
    // 결과 Stmt가 ExpressionStmt 이고
    // 내부 Expr 이 LiteralExpr(value=123.0) 인지 확인
}

TEST(ParserTest, VariableExpression) {
    // "a;" 을 파싱한다
    // 결과 Expr 이 VariableExpr(name="a") 인지 확인
}

TEST(ParserTest, BooleanLiteralExpression) {
    // "true;"  → BooleanLiteralExpr(value=true)
    // "false;" → BooleanLiteralExpr(value=false) 확인
}

TEST(ParserTest, UnaryExpression) {
    // "-a;" → UnaryExpr(op=MINUS, right=VariableExpr("a")) 확인
    // "!a;" → UnaryExpr(op=BANG,  right=VariableExpr("a")) 확인
}

TEST(ParserTest, AssignExpression) {
    // "a = 10;" 을 파싱한다
    // AssignExpr(name=Token("a"), value=LiteralExpr(10)) 인지 확인
}

TEST(ParserTest, BinaryExpression) {
    // "a + b;" → BinaryExpr(op=PLUS,    left=Variable("a"), right=Variable("b")) 확인
    // "a > b;" → BinaryExpr(op=GREATER, left=Variable("a"), right=Variable("b")) 확인
}

TEST(ParserTest, OperatorPrecedence_MulBeforeAdd) {
    // "a + b * 3;" 을 파싱한다
    // 루트가 BinaryExpr(+) 이고
    // 오른쪽 자식이 BinaryExpr(*) 인지 확인 (곱셈이 더 깊은 위치)
}

TEST(ParserTest, OperatorPrecedence_GroupingOverride) {
    // "(a + b) * 3;" 을 파싱한다
    // 루트가 BinaryExpr(*) 이고
    // 왼쪽 자식이 GroupingExpr → BinaryExpr(+) 인지 확인
}

TEST(ParserTest, VarDeclareStatement) {
    // "var x = 10;" 을 파싱한다
    // VarDeclareStmt(name=Token("x"), initializer=LiteralExpr(10)) 인지 확인
}

TEST(ParserTest, PrintStatement) {
    // "print a;" 을 파싱한다
    // PrintStmt(expr=VariableExpr("a")) 인지 확인
}

TEST(ParserTest, BlockStatement) {
    // "{ x = 1; y = 2; }" 을 파싱한다
    // BlockStmt 이고 내부 statements 가 2개인지 확인
    // 각각 ExpressionStmt(AssignExpr) 인지 확인
}

TEST(ParserTest, IfStatement) {
    // "if (a > 0) y = 1; else y = 2;" 을 파싱한다
    // IfStmt.condition  == BinaryExpr(>)
    // IfStmt.thenBranch == ExpressionStmt(AssignExpr("y", 1))
    // IfStmt.elseBranch == ExpressionStmt(AssignExpr("y", 2)) 확인
}

TEST(ParserTest, ForStatement) {
    // "for (var i = 0; i < 3; i = i + 1) {}" 을 파싱한다
    // ForStmt.init      == VarDeclareStmt("i", 0)
    // ForStmt.condition == BinaryExpr(<, Variable("i"), Literal(3))
    // ForStmt.increment == AssignExpr("i", BinaryExpr(+, Variable("i"), Literal(1)))
    // ForStmt.body      == BlockStmt([]) 확인
}

TEST(ParserTest, ExprCannotContainStmtAsChild) {
    // Expr 노드의 child 로 Stmt 노드가 포함되지 않는 구조인지 확인
    // 생성된 AST 를 순회하며 Expr 의 자식이 모두 Expr 타입인지 검사
}

// ═════════════════════════════════════════════════════════════
//  Checker Unit - 정적 오류 검사
// ═════════════════════════════════════════════════════════════

TEST(CheckerTest, ValidDeclarationNoError) {
    // "var a = 10; print a;" 을 파싱 후 Checker 에 전달한다
    // 예외가 발생하지 않는지 확인 (EXPECT_NO_THROW)
}

TEST(CheckerTest, ShadowingIsAllowed) {
    // "var x = 1; { var x = 2; print x; } print x;" 을 파싱 후 Checker 에 전달한다
    // 서로 다른 블록이므로 예외가 발생하지 않는지 확인 (EXPECT_NO_THROW)
}

TEST(CheckerTest, NestedScopeReadOuterVariable) {
    // "var outer = 1; { { print outer; } }" 을 파싱 후 Checker 에 전달한다
    // 외부 변수 읽기는 허용되므로 예외가 발생하지 않는지 확인 (EXPECT_NO_THROW)
}

TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    // "{ var a = a; }" 을 파싱 후 Checker 에 전달한다
    // "Can't read local variable in initializer." 류의 예외가 발생하는지 확인
    // EXPECT_THROW 또는 EXPECT_THAT(exception.what(), HasSubstr("..."))
}

TEST(CheckerTest, DuplicateVariableInSameScopeThrows) {
    // "{ var a = \"hi\"; var a = 3; }" 을 파싱 후 Checker 에 전달한다
    // "Already a variable with this name in this scope." 류의 예외가 발생하는지 확인
}

// ═════════════════════════════════════════════════════════════
//  Executor Unit - 표현식 / 연산자 / 우선순위
// ═════════════════════════════════════════════════════════════

TEST(ExecutorTest, ArithmeticPrecedenceMul) {
    // "print 1 + 2 * 3;" 을 전체 파이프라인으로 실행한다
    // stdout 출력이 "7" 임을 확인
}

TEST(ExecutorTest, ArithmeticPrecedenceGrouping) {
    // "print (1 + 2) * 3;" 을 실행한다
    // stdout 출력이 "9" 임을 확인
}

TEST(ExecutorTest, ArithmeticLeftAssociativity_Sub) {
    // "print 10 - 4 - 3;" 을 실행한다
    // stdout 출력이 "3" 임을 확인 (좌결합: (10-4)-3 = 3)
}

TEST(ExecutorTest, ArithmeticLeftAssociativity_Div) {
    // "print 8 / 2 / 2;" 을 실행한다
    // stdout 출력이 "2" 임을 확인 (좌결합: (8/2)/2 = 2)
}

TEST(ExecutorTest, UnaryMinus) {
    // "print -3 + 2;" 을 실행한다
    // stdout 출력이 "-1" 임을 확인
}

TEST(ExecutorTest, ComparisonLessThan) {
    // "print 1 < 2;" 을 실행한다
    // stdout 출력이 "true" 임을 확인
}

TEST(ExecutorTest, ComparisonGreaterThan) {
    // "print 3 > 5;" 을 실행한다
    // stdout 출력이 "false" 임을 확인
}

TEST(ExecutorTest, StringConcatenation) {
    // "print \"Hello, \" + \"CodeFab!\";" 을 실행한다
    // stdout 출력이 "Hello, CodeFab!" 임을 확인
}

TEST(ExecutorTest, IntegerPrintFormat) {
    // "print 5;"   → 출력이 "5"  (.0 없음) 확인
    // "print 5.0;" → 출력이 "5"  (.0 없음) 확인
}

TEST(ExecutorTest, FloatPrintFormat) {
    // "print 3.14;" 을 실행한다
    // stdout 출력이 "3.14" 임을 확인
}

TEST(ExecutorTest, BooleanLiteralPrint) {
    // "print true;"  → 출력이 "true"  확인
    // "print false;" → 출력이 "false" 확인
}

// ═════════════════════════════════════════════════════════════
//  Executor Unit - 변수 / 할당 / 블록 스코프 / Shadowing
// ═════════════════════════════════════════════════════════════

TEST(ExecutorTest, VarDeclarationAndUse) {
    // "var a = 10; var b = 20; print a + b;" 을 실행한다
    // stdout 출력이 "30" 임을 확인
}

TEST(ExecutorTest, VarReassignment) {
    // "var a = 10; a = a + 5; print a;" 을 실행한다
    // stdout 출력이 "15" 임을 확인
}

TEST(ExecutorTest, BlockScopeAndShadowing) {
    // "var x = \"global\"; { var x = \"inner\"; print x; } print x;"
    // 을 실행한다
    // stdout 출력이 "inner\nglobal" 임을 확인
}

TEST(ExecutorTest, BlockModifiesOuterVariable) {
    // "var count = 0; { count = count + 1; } print count;" 을 실행한다
    // stdout 출력이 "1" 임을 확인
    // (블록 안에서 바깥 변수를 수정할 수 있어야 한다)
}

TEST(ExecutorTest, NestedScopeVariableAccess) {
    // "var outer = \"A\"; { var inner = \"B\"; { print outer + inner; } }"
    // 을 실행한다
    // stdout 출력이 "AB" 임을 확인
}

// ═════════════════════════════════════════════════════════════
//  Executor Unit - 제어 흐름 (if / else / for)
// ═════════════════════════════════════════════════════════════

TEST(ExecutorTest, IfTrue) {
    // "if (true) print \"bbq\";" 을 실행한다
    // stdout 출력이 "bbq" 임을 확인
}

TEST(ExecutorTest, IfFalseElse) {
    // "if (false) print \"no\"; else print \"kfc\";" 을 실행한다
    // stdout 출력이 "kfc" 임을 확인
}

TEST(ExecutorTest, DanglingElseBindsToNearest) {
    // "if (true) if (false) print \"kfc\"; else print \"bbq\";" 을 실행한다
    // stdout 출력이 "bbq" 임을 확인
    // (else 는 바깥 if 가 아닌 안쪽 if 에 결합해야 한다)
}

TEST(ExecutorTest, ForLoop) {
    // "for (var j = 0; j < 3; j = j + 1) { print j; }" 을 실행한다
    // stdout 출력이 "0\n1\n2" 임을 확인
}

// ═════════════════════════════════════════════════════════════
//  에러 검출 - 구문 에러 (Assembler Unit)
// ═════════════════════════════════════════════════════════════

TEST(SyntaxErrorTest, MissingSemicolon) {
    // "print 1 + 2" (세미콜론 없음) 을 파이프라인에 전달한다
    // "Expect ';' after value." 류의 예외가 발생하는지 확인
}

TEST(SyntaxErrorTest, MissingClosingParen) {
    // "print (1 + 2;" (닫는 괄호 없음) 을 파이프라인에 전달한다
    // "Expect ')' after expression." 류의 예외가 발생하는지 확인
}

TEST(SyntaxErrorTest, InvalidAssignmentTarget) {
    // "var a = 1; var b = 2; a + b = 3;" 을 파이프라인에 전달한다
    // "Invalid assignment target." 류의 예외가 발생하는지 확인
}

TEST(SyntaxErrorTest, UnexpectedTokenInExpression) {
    // "print * 5;" 을 파이프라인에 전달한다
    // "Expect expression." 류의 예외가 발생하는지 확인
}

// ═════════════════════════════════════════════════════════════
//  에러 검출 - 런타임 에러 (Executor Unit)
// ═════════════════════════════════════════════════════════════

TEST(RuntimeErrorTest, UndefinedVariable) {
    // "print notDefined;" 을 실행한다
    // "Undefined variable 'notDefined'." 류의 예외가 발생하는지 확인
    // 줄 번호 정보가 메시지에 포함되는지도 확인
}

TEST(RuntimeErrorTest, AddNumberAndString) {
    // "print 1 + \"HI\";" 을 실행한다
    // "Operands must be two numbers or two strings." 류의 예외가 발생하는지 확인
}

TEST(RuntimeErrorTest, UnaryMinusOnString) {
    // "print -\"FabCoding\";" 을 실행한다
    // "Operand must be a number." 류의 예외가 발생하는지 확인
}

TEST(RuntimeErrorTest, DivisionByZero) {
    // "var a = 3 / 0;" 을 실행한다
    // "0으로 나눈 오류" 류의 예외가 발생하는지 확인
}

TEST(RuntimeErrorTest, BooleanArithmeticOperation) {
    // "print true * false;" 을 실행한다
    // boolean 타입에 * 연산은 지원하지 않으므로 예외가 발생하는지 확인
}

// ═════════════════════════════════════════════════════════════
//  통합 파이프라인 (Assembler → Checker → Executor)
// ═════════════════════════════════════════════════════════════

TEST(PipelineTest, ForWithIfInside) {
    // "var a = 5;"
    // "for (var i = 0; i < 2; i = i + 1) { if (a > 3) a = a - 1; }"
    // "print a;"
    // 을 3-Unit 파이프라인으로 실행한다
    // stdout 출력이 "3" 임을 확인 (반복 2회, 매회 a>3이면 a-1)
}

TEST(PipelineTest, BlockVariableLifetime) {
    // "var result = 0;"
    // "{ var tmp = 10; result = tmp * 2; }"
    // "print result;"
    // 을 실행한다
    // stdout 출력이 "20" 임을 확인
    // (블록 종료 후 tmp 는 소멸, result 는 유지)
}

TEST(PipelineTest, StringVariableConcatenation) {
    // "var greeting = \"Score: \";"
    // "print greeting + \"100\";"
    // 을 실행한다
    // stdout 출력이 "Score: 100" 임을 확인
}
