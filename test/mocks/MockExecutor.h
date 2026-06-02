#pragma once
#include <gmock/gmock.h>
#include "src/executor/IExecutor.h"

struct MockExecutor : IExecutor {
    MOCK_METHOD(void, execute,
                (const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out),
                (override));
};
