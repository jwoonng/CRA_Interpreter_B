#pragma once
#include <stdexcept>
#include <string>
#include "IChecker.h"
#include <unordered_map>

// CheckError: Checker가 발생시키는 의미 분석 오류
struct CheckError : std::runtime_error {
    int line;
    CheckError(int line, const std::string& msg)
        : std::runtime_error("[" + std::to_string(line) + "번째 줄] " + msg), line(line) {}
};

class Checker : public IChecker {
public:
    void check(const std::vector<std::unique_ptr<Stmt>>& stmts) override;

private:
    // false = 선언됨(미초기화), true = 초기화 완료
    std::vector<std::unordered_map<std::string, bool>> scopes_;

    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name);
    void resolveVar(const Token& name);
    void checkStmt(Stmt& s);
    void checkExpr(Expr& e);
};
