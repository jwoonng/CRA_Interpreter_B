#pragma once
#include "src/executor/IExecutor.h"
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <vector>

// quit / exit 명령어 시 throw — Shell::runDebug() 에서 catch 하여 정상 종료
struct DebugQuitRequest {};

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
//   watch <name>[<idx>]   watch a specific array element (e.g. watch arr[0])
//   unwatch <name>        remove a variable from the watch list
//   unwatch <name>[<idx>] remove an indexed array element watch
//   watches               print watched variables (nearest scope value)
//   inspect               print every variable in the current scope chain
//   quit / exit           terminate the debug session immediately
class Debugger : public DebugObserver {
public:
    Debugger(IExecutor& executor,
             std::vector<std::string> sourceLines,
             std::istream& in,
             std::ostream& out);

    void beforeStatement(const Stmt& stmt, int depth) override;

private:
    enum class Mode { Step, Next, Run };

    struct IndexedWatch {
        std::string name;
        int         index;
    };

    IExecutor&               executor_;
    std::vector<std::string> sourceLines_;
    std::istream&            in_;
    std::ostream&            out_;

    Mode                     mode_      = Mode::Step;  // stop at the first statement
    int                      nextDepth_ = 0;           // target depth for "next"
    std::set<int>            breakpoints_;
    std::vector<std::string> watches_;                 // plain variable watches, insertion order
    std::vector<IndexedWatch> indexedWatches_;         // arr[N] watches, insertion order

    bool        shouldPause(const Stmt& stmt, int depth, bool& isBreakpoint) const;
    void        reportStop(const Stmt& stmt, bool isBreakpoint);
    void        reportWatches();
    std::string resolveIndexedWatch(const std::string& name, int index) const;
    void        commandLoop(int depth);
    std::string sourceText(int line) const;

    void cmdBreak(const std::string& arg);
    void cmdRemove(const std::string& arg);
    void cmdBreakpoints();
    void cmdWatch(const std::string& arg);
    void cmdUnwatch(const std::string& arg);
    void cmdWatches();
    void cmdInspect();
};
