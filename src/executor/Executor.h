#pragma once
#include "IExecutor.h"
#include "src/common/Expr.h"
#include "src/common/Stmt.h"
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

// ── 스코프 환경 ──────────────────────────────────────────────────
struct Environment {
    std::map<std::string, LiteralValue> values;
    Environment* enclosing = nullptr;

    explicit Environment(Environment* enc = nullptr) : enclosing(enc) {}

    void define(const std::string& name, LiteralValue val) {
        values[name] = std::move(val);
    }

    LiteralValue get(const std::string& name, int line = 0) {
        auto it = values.find(name);
        if (it != values.end()) return it->second;
        if (enclosing) return enclosing->get(name, line);
        throw std::runtime_error("[line " + std::to_string(line) + "] Undefined variable '" + name + "'.");
    }

    void assign(const std::string& name, LiteralValue val, int line = 0) {
        auto it = values.find(name);
        if (it != values.end()) { it->second = std::move(val); return; }
        if (enclosing) { enclosing->assign(name, std::move(val), line); return; }
        throw std::runtime_error("[line " + std::to_string(line) + "] Undefined variable '" + name + "'.");
    }

    bool contains(const std::string& name) const {
        if (values.count(name)) return true;
        if (enclosing) return enclosing->contains(name);
        return false;
    }

    // StaticBindingOptimizer가 계산한 distance 기반 O(1) 직접 접근
    LiteralValue getAt(int distance, const std::string& name) {
        auto* env = ancestor(distance);
        auto  it  = env->values.find(name);
        if (it != env->values.end()) return it->second;
        throw std::runtime_error("Undefined variable '" + name + "'.");
    }

    void assignAt(int distance, const std::string& name, LiteralValue val) {
        auto* env = ancestor(distance);
        auto  it  = env->values.find(name);
        if (it == env->values.end())
            throw std::runtime_error(
                "Internal error: variable '" + name +
                "' not found at distance " + std::to_string(distance) + ".");
        it->second = std::move(val);
    }

    Environment* ancestor(int distance) {
        auto* env = this;
        for (int i = 0; i < distance; ++i) {
            if (!env->enclosing)
                throw std::runtime_error(
                    "Internal error: scope chain shorter than expected (distance=" +
                    std::to_string(distance) + ").");
            env = env->enclosing;
        }
        return env;
    }
};

// ── Function storage (raw, non-owning: valid during execute()) ────────
struct FunctionEntry {
    const FunctionStmt* decl;
    Environment*        closure;
};

// ── Return control flow ───────────────────────────────────────────────
struct ReturnException {
    LiteralValue value;
};

// ── Executor ─────────────────────────────────────────────────────────
class Executor : public IExecutor, private ExprVisitor, private StmtVisitor {
public:
    void execute(std::vector<std::unique_ptr<Stmt>> stmts,
                 std::ostream& out) override;

private:
    Environment  global_;
    Environment* env_ = &global_;
    std::ostream* out_ = nullptr;
    std::unordered_map<std::string, FunctionEntry> functions_;
    std::vector<std::vector<std::unique_ptr<Stmt>>> ownedStmts_;

    LiteralValue evaluate(Expr& e);
    void         run(Stmt& s);
    std::string  stringify(const LiteralValue& v);

    // ExprVisitor
    LiteralValue visitLiteralExpr(LiteralExpr& e)   override;
    LiteralValue visitVariableExpr(VariableExpr& e) override;
    LiteralValue visitAssignExpr(AssignExpr& e)     override;
    LiteralValue visitBinaryExpr(BinaryExpr& e)     override;
    LiteralValue visitUnaryExpr(UnaryExpr& e)       override;
    LiteralValue visitGroupingExpr(GroupingExpr& e) override;
    LiteralValue visitLogicalExpr(LogicalExpr& e)   override;

    // StmtVisitor
    void visitExpressionStmt(ExpressionStmt& s) override;
    void visitPrintStmt(PrintStmt& s)           override;
    void visitVarDeclareStmt(VarDeclareStmt& s) override;
    void visitBlockStmt(BlockStmt& s)           override;
    void visitIfStmt(IfStmt& s)                 override;
    void visitForStmt(ForStmt& s)               override;
    void visitFunctionStmt(FunctionStmt& s)     override;
    void visitReturnStmt(ReturnStmt& s)         override;

    // ExprVisitor (function call)
    LiteralValue visitCallExpr(CallExpr& e)          override;

    // ExprVisitor (array)
    LiteralValue visitIndexExpr(IndexExpr& e)             override;
    LiteralValue visitIndexAssignExpr(IndexAssignExpr& e) override;
    LiteralValue callBuiltinArray(CallExpr& e);
};
