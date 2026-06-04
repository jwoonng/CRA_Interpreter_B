#pragma once
#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
#include "src/common/Stmt.h"

// Checker Runtime error 처리
struct CheckError : std::runtime_error {
    int line;
    CheckError(int line, const std::string& msg)
        : std::runtime_error("[" + std::to_string(line) + "번째 줄] " + msg), line(line) {}
};

// Checker 정적 오류 검사 인터페이스
struct IChecker {
    virtual ~IChecker() = default;

    // 오류 발견 시 CheckError(std::runtime_error 계열) 예외를 던진다
    virtual void check(const std::vector<std::unique_ptr<Stmt>>& stmts) = 0;
};
