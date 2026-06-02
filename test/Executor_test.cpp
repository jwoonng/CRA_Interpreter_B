#include <gtest/gtest.h>
#include <sstream>
#include "src/executor/Executor.h"
// MockChecker 는 Checker_test.cpp 전용 — Executor 는 AST 를 직접 받으므로 불필요

// ── 개발자 4 담당 영역 ──────────────────────────────

TEST(ExecutorTest, PrintLiteral) {
    // TODO
}

TEST(ExecutorTest, ArithmeticPrecedence) {
    // TODO
}

TEST(ExecutorTest, VarDeclAndUse) {
    // TODO
}

TEST(ExecutorTest, BlockScopeAndShadowing) {
    // TODO
}

TEST(ExecutorTest, IfTrue) {
    // TODO
}

TEST(ExecutorTest, IfFalseElse) {
    // TODO
}

TEST(ExecutorTest, ForLoop) {
    // TODO
}

TEST(ExecutorTest, UndefinedVariableThrows) {
    // TODO
}

TEST(ExecutorTest, DivisionByZeroThrows) {
    // TODO
}

TEST(ExecutorTest, TypeMismatchThrows) {
    // TODO
}
