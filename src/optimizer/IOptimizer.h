#pragma once
#include "src/common/Stmt.h"
#include <memory>
#include <vector>

// ── IOptimizer — 최적화 패스 인터페이스 (Strategy Pattern) ───────
// AST를 받아 최적화된 AST를 반환한다.
// 여러 Optimizer를 Shell 파이프라인에 체인으로 연결할 수 있다 (Chain of Responsibility).
//
// 구현 예시:
//   - ConstantFoldingOptimizer : 1 + 2 * 3 → LiteralExpr(7) 으로 사전 계산
//   - DeadCodeOptimizer        : if (false) { ... } 제거
//   - InliningOptimizer        : 단순 함수 인라인 치환
struct IOptimizer {
    virtual ~IOptimizer() = default;

    // stmts 를 받아 최적화된 AST 를 반환 (소유권 이전)
    virtual std::vector<std::unique_ptr<Stmt>> optimize(
        std::vector<std::unique_ptr<Stmt>> stmts) = 0;
};
