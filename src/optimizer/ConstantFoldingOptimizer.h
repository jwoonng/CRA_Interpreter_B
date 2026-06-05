#pragma once
#include "IOptimizer.h"
#include "src/common/Expr.h"
#include "src/common/Stmt.h"

struct ConstantFoldingOptimizer : IOptimizer {
    std::vector<StmtPtr> optimize(std::vector<StmtPtr> stmts) override;

private:
    bool         isConst(Expr* e);
    LiteralValue eval(Expr* e);
    ExprPtr      foldExpr(ExprPtr expr);
    void         foldStmt(StmtPtr& stmt);
};
