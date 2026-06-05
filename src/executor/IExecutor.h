#pragma once
#include <memory>
#include <ostream>
#include <vector>
#include "src/common/Stmt.h"

// Executor Unit — 런타임 실행 인터페이스
// 담당: 개발자 4
struct IExecutor {
    virtual ~IExecutor() = default;

    // 문법 Tree 실행, 출력은 out 스트림으로
    // 런타임 오류 시 std::runtime_error 계열 예외를 던진다
    virtual void execute(std::vector<std::unique_ptr<Stmt>> stmts, std::ostream& output) = 0;
};
