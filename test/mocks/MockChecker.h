#pragma once
#include <gmock/gmock.h>
#include "src/checker/IChecker.h"

struct MockChecker : IChecker {
    MOCK_METHOD(void, check,
                (const std::vector<std::unique_ptr<Stmt>>& stmts), (override));
};
