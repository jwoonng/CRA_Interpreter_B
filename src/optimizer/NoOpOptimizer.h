#pragma once
#include "IOptimizer.h"

// ── NoOpOptimizer — Null Object Pattern ──────────────────────────
// 아무 변환도 하지 않고 그대로 반환한다.
// 기본 파이프라인에 사용되며, 새로운 Optimizer 구현의 기반이 된다.
struct NoOpOptimizer : IOptimizer {
    std::vector<std::unique_ptr<Stmt>> optimize(
        std::vector<std::unique_ptr<Stmt>> stmts) override {
        return std::move(stmts);
    }
};
