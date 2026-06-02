#pragma once
#include "IExecutor.h"

class Executor : public IExecutor {
public:
    void execute(const std::vector<std::unique_ptr<Stmt>>& stmts,
        std::ostream& out);
};
