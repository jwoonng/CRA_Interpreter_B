#pragma once
#include <memory>
#include <vector>
#include "src/common/Stmt.h"

// Checker Unit — 정적 오류 검사 인터페이스
// 담당: 개발자 3
struct IChecker {
    virtual ~IChecker() = default;

    // 오류 발견 시 std::runtime_error 계열 예외를 던진다
    virtual void check(const std::vector<std::unique_ptr<Stmt>>& stmts) = 0;
};
