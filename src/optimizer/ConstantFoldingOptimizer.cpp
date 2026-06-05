#include "ConstantFoldingOptimizer.h"
#include <cmath>
#include <stdexcept>

std::vector<StmtPtr> ConstantFoldingOptimizer::optimize(std::vector<StmtPtr> stmts) {
    for (auto& s : stmts)
        foldStmt(s);
    return std::move(stmts);
}

// ── 상수 여부 판별 ──────────────────────────────────────────────────────────
bool ConstantFoldingOptimizer::isConst(Expr* e) {
    if (!e) return false;
    if (dynamic_cast<LiteralExpr*>(e))              return true;
    if (auto* g = dynamic_cast<GroupingExpr*>(e))   return isConst(g->expression.get());
    if (auto* u = dynamic_cast<UnaryExpr*>(e))      return isConst(u->right.get());
    if (auto* b = dynamic_cast<BinaryExpr*>(e))     return isConst(b->left.get()) && isConst(b->right.get());
    return false;
}

// ── 상수 식 평가 ────────────────────────────────────────────────────────────
LiteralValue ConstantFoldingOptimizer::eval(Expr* e) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(e))
        return lit->value;

    if (auto* g = dynamic_cast<GroupingExpr*>(e))
        return eval(g->expression.get());

    if (auto* u = dynamic_cast<UnaryExpr*>(e)) {
        auto val = eval(u->right.get());
        const int line = u->op.line;
        if (u->op.type == TokenType::MINUS) {
            if (auto* d = std::get_if<double>(&val)) return -*d;
            throw std::runtime_error("[line " + std::to_string(line) + "] Operand must be a number.");
        }
        if (u->op.type == TokenType::BANG) {
            if (auto* b = std::get_if<bool>(&val)) return !*b;
            throw std::runtime_error("[line " + std::to_string(line) + "] Operand must be a boolean.");
        }
    }

    if (auto* b = dynamic_cast<BinaryExpr*>(e)) {
        auto lv = eval(b->left.get());
        auto rv = eval(b->right.get());
        auto* ld = std::get_if<double>(&lv);
        auto* rd = std::get_if<double>(&rv);
        auto* ls = std::get_if<std::string>(&lv);
        auto* rs = std::get_if<std::string>(&rv);
        const int line = b->op.line;

        switch (b->op.type) {
        case TokenType::PLUS:
            if (ld && rd) return *ld + *rd;
            if (ls && rs) return *ls + *rs;
            throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be two numbers or two strings.");
        case TokenType::MINUS:
            if (ld && rd) return *ld - *rd;
            throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::STAR:
            if (ld && rd) return *ld * *rd;
            throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::SLASH:
            if (ld && rd) {
                if (*rd == 0.0) throw std::runtime_error("[line " + std::to_string(line) + "] Division by zero.");
                return *ld / *rd;
            }
            throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::PERCENT:
            if (ld && rd) {
                if (*rd == 0.0) throw std::runtime_error("[line " + std::to_string(line) + "] Division by zero.");
                return std::fmod(*ld, *rd);
            }
            throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::GREATER:       if (ld && rd) return *ld > *rd;  throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::GREATER_EQUAL: if (ld && rd) return *ld >= *rd; throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::LESS:          if (ld && rd) return *ld < *rd;  throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::LESS_EQUAL:    if (ld && rd) return *ld <= *rd; throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
        case TokenType::EQUAL_EQUAL:   return lv == rv;
        case TokenType::BANG_EQUAL:    return lv != rv;
        default: throw std::runtime_error("[line " + std::to_string(line) + "] Unknown binary operator.");
        }
    }

    throw std::runtime_error("[line " + std::to_string(e->line) + "] Cannot evaluate constant expression.");
}

// ── Expr 접기: 상수이면 LiteralExpr로 교체, 아니면 자식 재귀 ───────────────
ExprPtr ConstantFoldingOptimizer::foldExpr(ExprPtr expr) {
    if (!expr) return expr;

    if (isConst(expr.get())) {
        try {
            return std::make_unique<LiteralExpr>(eval(expr.get()), expr->line);
        } catch (...) {
            // 0 나누기 등 평가 불가 — 원본 유지
        }
    }

    if (auto* g = dynamic_cast<GroupingExpr*>(expr.get())) {
        g->expression = foldExpr(std::move(g->expression));
    } else if (auto* u = dynamic_cast<UnaryExpr*>(expr.get())) {
        u->right = foldExpr(std::move(u->right));
    } else if (auto* b = dynamic_cast<BinaryExpr*>(expr.get())) {
        b->left  = foldExpr(std::move(b->left));
        b->right = foldExpr(std::move(b->right));
    } else if (auto* a = dynamic_cast<AssignExpr*>(expr.get())) {
        a->value = foldExpr(std::move(a->value));
    } else if (auto* l = dynamic_cast<LogicalExpr*>(expr.get())) {
        l->left  = foldExpr(std::move(l->left));
        l->right = foldExpr(std::move(l->right));
    }
    return expr;
}

// ── Stmt 접기 ───────────────────────────────────────────────────────────────
void ConstantFoldingOptimizer::foldStmt(StmtPtr& stmt) {
    if (!stmt) return;

    if (auto* s = dynamic_cast<ExpressionStmt*>(stmt.get())) {
        s->expression = foldExpr(std::move(s->expression));
    } else if (auto* s = dynamic_cast<PrintStmt*>(stmt.get())) {
        s->expression = foldExpr(std::move(s->expression));
    } else if (auto* s = dynamic_cast<VarDeclareStmt*>(stmt.get())) {
        s->initializer = foldExpr(std::move(s->initializer));
    } else if (auto* s = dynamic_cast<BlockStmt*>(stmt.get())) {
        for (auto& inner : s->statements) foldStmt(inner);
    } else if (auto* s = dynamic_cast<IfStmt*>(stmt.get())) {
        s->condition  = foldExpr(std::move(s->condition));
        foldStmt(s->thenBranch);
        foldStmt(s->elseBranch);
    } else if (auto* s = dynamic_cast<ForStmt*>(stmt.get())) {
        foldStmt(s->initializer);
        s->condition  = foldExpr(std::move(s->condition));
        s->increment  = foldExpr(std::move(s->increment));
        foldStmt(s->body);
    }
}
