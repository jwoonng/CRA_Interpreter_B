#pragma once
#include "IChecker.h"         // → src/common/Stmt.h → src/common/Expr.h → Token.h
#include <unordered_map>
#include <string>

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
