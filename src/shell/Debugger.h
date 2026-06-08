#pragma once
#include "src/executor/Executor.h"
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <vector>

// ── Debugger — statement-level stepping driver (factory "debug" mode) ──
//
// Implements DebugObserver: the Executor invokes beforeStatement() before
// every stoppable statement. When the debugger decides to pause it prints
// the current location, reports any watched variables, then reads commands
// from its input stream until a resume command (step / next / continue).
//
// Commands:
//   step                  run current statement, stop at the next one
//   next                  run current statement without stepping into blocks
//   break <line>          set a breakpoint at <line>
//   breakpoints           list active breakpoints
//   remove <line>         clear the breakpoint at <line>
//   continue              run until the next breakpoint
//   watch <name>          add a variable to the watch list
//   unwatch <name>        remove a variable from the watch list
//   watches               print watched variables (nearest scope value)
//   inspect               print every variable in the current scope chain
class Debugger : public DebugObserver {
public:
    Debugger(Executor& executor,
             std::vector<std::string> sourceLines,
             std::istream& in,
             std::ostream& out);

    void beforeStatement(const Stmt& stmt, int depth) override;

private:
    enum class Mode { Step, Next, Run };

    Executor&                executor_;
    std::vector<std::string> sourceLines_;
    std::istream&            in_;
    std::ostream&            out_;

    Mode                     mode_      = Mode::Step;  // stop at the first statement
    int                      nextDepth_ = 0;           // target depth for "next"
    std::set<int>            breakpoints_;
    std::vector<std::string> watches_;                 // insertion order preserved

    bool        shouldPause(const Stmt& stmt, int depth, bool& isBreakpoint) const;
    void        reportStop(const Stmt& stmt, bool isBreakpoint);
    void        reportWatches();
    void        commandLoop(int depth);
    std::string sourceText(int line) const;

    void cmdBreak(const std::string& arg);
    void cmdRemove(const std::string& arg);
    void cmdBreakpoints();
    void cmdWatch(const std::string& name);
    void cmdUnwatch(const std::string& name);
    void cmdWatches();
    void cmdInspect();
};
