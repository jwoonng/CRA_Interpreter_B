#pragma once
#include <memory>
#include <vector>
#include "src/common/Stmt.h"

// Checker 정적 오류 검사 인터페이스
struct IChecker {
    virtual ~IChecker() = default;

    // 오류 발견 시 CheckError(std::runtime_error 계열) 예외를 던진다
    virtual void check(const std::vector<std::unique_ptr<Stmt>>& stmts) = 0;

    // REPL 전용 — executor 런타임 에러 발생 시 마지막 check() 에서 추가된
    // 전역 선언을 되돌린다. 기본 구현은 no-op.
    virtual void rollbackLastCheck() {}
};
