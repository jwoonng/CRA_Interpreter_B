#pragma once
#include "IChecker.h"

class Checker : public IChecker {
public:
    void check(const std::vector<std::unique_ptr<Stmt>>& stmts);
};
