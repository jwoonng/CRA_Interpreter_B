#include <gtest/gtest.h>
#include "src/checker/Checker.h"
// MockParser 는 Parser_test.cpp 전용 — Checker 는 AST 를 직접 받으므로 불필요

// ── 개발자 3 담당 영역 ──────────────────────────────

TEST(CheckerTest, ValidCodeNoThrow) {
    // TODO
}

TEST(CheckerTest, ShadowingAllowed) {
    // TODO
}

TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    // TODO
}

TEST(CheckerTest, DuplicateVarInSameScopeThrows) {
    // TODO
}
