#pragma once
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include "src/common/Stmt.h"

// ── Debug stepping support (factory "debug" mode) ─────────────────────
// An Executor notifies an observer before every stoppable statement.
// "depth" is the current block-nesting level (0 = top level), used by the
// debugger to implement step-over ("next").
struct DebugObserver {
    virtual ~DebugObserver() = default;
    virtual void beforeStatement(const Stmt& stmt, int depth) = 0;
};

// A single variable visible at a pause point, for the debugger's
// "inspect" / "watches" commands.
struct DebugVar {
    bool         isGlobal;
    std::string  name;
    LiteralValue value;
};

// Executor Unit — 런타임 실행 인터페이스
// 담당: 개발자 4
struct IExecutor {
    virtual ~IExecutor() = default;

    // 문법 Tree 실행, 출력은 out 스트림으로
    // 런타임 오류 시 std::runtime_error 계열 예외를 던진다
    virtual void execute(std::vector<std::unique_ptr<Stmt>> stmts, std::ostream& output) = 0;

    // ── Debug hooks (default: no-op, so non-debugging executors and test
    //    doubles need not implement them; the built-in Executor overrides) ──
    virtual void setDebugObserver(DebugObserver* /*observer*/) {}
    virtual bool debugLookup(const std::string& /*name*/, LiteralValue& /*out*/) const { return false; }
    virtual std::vector<DebugVar> debugSnapshot() const { return {}; }
};
