#include "Debugger.h"
#include <cmath>
#include <limits>
#include <sstream>
#include <unordered_set>

// ── value formatting helpers ──────────────────────────────────────────
namespace {

std::string formatDouble(double d) {
    constexpr double LLONG_MIN_D = static_cast<double>(std::numeric_limits<long long>::min());
    constexpr double LLONG_MAX_D = static_cast<double>(std::numeric_limits<long long>::max());
    if (d == std::floor(d) && d >= LLONG_MIN_D && d <= LLONG_MAX_D)
        return std::to_string(static_cast<long long>(d));
    std::ostringstream oss;
    oss << d;
    return oss.str();
}

std::string stringifyValue(const LiteralValue& v,
                           std::unordered_set<const LiteralArray*>& visited) {
    return std::visit([&visited](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "nil";
        if constexpr (std::is_same_v<T, bool>)           return val ? "true" : "false";
        if constexpr (std::is_same_v<T, std::string>)    return val;
        if constexpr (std::is_same_v<T, double>)         return formatDouble(val);
        if constexpr (std::is_same_v<T, ArrayPtr>) {
            if (!val) return "nil";
            if (visited.count(val.get())) return "[...]";  // 순환 참조 감지
            visited.insert(val.get());
            std::string result = "[";
            for (std::size_t i = 0; i < val->elements.size(); ++i) {
                if (i > 0) result += ", ";
                result += stringifyValue(val->elements[i], visited);
            }
            visited.erase(val.get());
            return result + "]";
        }
    }, v);
}

std::string stringifyValue(const LiteralValue& v) {
    std::unordered_set<const LiteralArray*> visited;
    return stringifyValue(v, visited);
}

std::string typeName(const LiteralValue& v) {
    return std::visit([](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "Nil";
        if constexpr (std::is_same_v<T, bool>)           return "Boolean";
        if constexpr (std::is_same_v<T, std::string>)    return "String";
        if constexpr (std::is_same_v<T, double>)         return "Number";
        if constexpr (std::is_same_v<T, ArrayPtr>)       return "Array";
    }, v);
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Parse "name[index]" → true and fill out name/index; false if not that format.
bool parseIndexedArg(const std::string& arg, std::string& name, int& index) {
    auto lb = arg.find('[');
    auto rb = arg.find(']');
    if (lb == std::string::npos || rb == std::string::npos || rb < lb) return false;
    name = arg.substr(0, lb);
    if (name.empty()) return false;
    try {
        index = std::stoi(arg.substr(lb + 1, rb - lb - 1));
    } catch (...) {
        return false;
    }
    return true;
}

}  // namespace

// ── construction ──────────────────────────────────────────────────────
Debugger::Debugger(IExecutor& executor,
                   std::vector<std::string> sourceLines,
                   std::istream& in,
                   std::ostream& out)
    : executor_(executor)
    , sourceLines_(std::move(sourceLines))
    , in_(in)
    , out_(out) {}

// ── observer entry point ──────────────────────────────────────────────
void Debugger::beforeStatement(const Stmt& stmt, int depth) {
    bool isBreakpoint = false;
    if (!shouldPause(stmt, depth, isBreakpoint)) return;
    reportStop(stmt, isBreakpoint);
    reportWatches();
    commandLoop(depth);
}

bool Debugger::shouldPause(const Stmt& stmt, int depth, bool& isBreakpoint) const {
    isBreakpoint = breakpoints_.count(stmt.line) > 0;
    switch (mode_) {
        case Mode::Step: return true;
        case Mode::Next: return depth <= nextDepth_ || isBreakpoint;
        case Mode::Run:  return isBreakpoint;
    }
    return false;
}

// ── reporting ─────────────────────────────────────────────────────────
std::string Debugger::sourceText(int line) const {
    if (line >= 1 && line <= static_cast<int>(sourceLines_.size()))
        return trim(sourceLines_[line - 1]);
    return "";
}

void Debugger::reportStop(const Stmt& stmt, bool isBreakpoint) {
    out_ << "[DEBUG] stopped at line " << stmt.line;
    if (isBreakpoint) out_ << " (breakpoint)";
    out_ << " -> " << sourceText(stmt.line) << "\n";
}

std::string Debugger::resolveIndexedWatch(const std::string& name, int index) const {
    std::string label = name + "[" + std::to_string(index) + "]";
    LiteralValue arrVal;
    if (!executor_.debugLookup(name, arrVal))
        return label + " = <undefined>";
    const auto* ap = std::get_if<ArrayPtr>(&arrVal);
    if (!ap || !*ap)
        return label + " = <not an array>";
    if (index < 0 || index >= static_cast<int>((*ap)->elements.size()))
        return label + " = <index out of range>";
    return label + " = " + stringifyValue((*ap)->elements[index]);
}

void Debugger::reportWatches() {
    for (const auto& name : watches_) {
        LiteralValue value;
        if (executor_.debugLookup(name, value))
            out_ << "[WATCH] " << name << " = " << stringifyValue(value) << "\n";
        else
            out_ << "[WATCH] " << name << " = <undefined>\n";
    }
    for (const auto& w : indexedWatches_)
        out_ << "[WATCH] " << resolveIndexedWatch(w.name, w.index) << "\n";
}

// ── command loop ──────────────────────────────────────────────────────
void Debugger::commandLoop(int depth) {
    std::string line;
    while (true) {
        out_ << "> ";
        if (!std::getline(in_, line)) {  // EOF — let the program run to the end
            mode_ = Mode::Run;
            return;
        }

        std::istringstream iss(trim(line));
        std::string cmd;
        iss >> cmd;
        std::string args;
        std::getline(iss, args);
        args = trim(args);

        if (cmd.empty())                          continue;
        if (cmd == "step" || cmd == "s")          { mode_ = Mode::Step; return; }
        if (cmd == "next" || cmd == "n")          { mode_ = Mode::Next; nextDepth_ = depth; return; }
        if (cmd == "continue" || cmd == "c")      { mode_ = Mode::Run;  return; }
        if (cmd == "quit"  || cmd == "exit")      { out_ << "[DEBUG] session terminated.\n"; throw DebugQuitRequest{}; }
        if (cmd == "break")                       cmdBreak(args);
        else if (cmd == "remove")                 cmdRemove(args);
        else if (cmd == "breakpoints")            cmdBreakpoints();
        else if (cmd == "watch")                  cmdWatch(args);
        else if (cmd == "unwatch")                cmdUnwatch(args);
        else if (cmd == "watches")                cmdWatches();
        else if (cmd == "inspect")                cmdInspect();
        else                                      out_ << "[DEBUG] unknown command: " << cmd << "\n";
    }
}

// ── breakpoint commands ───────────────────────────────────────────────
void Debugger::cmdBreak(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    bool any = false;
    while (iss >> token) {
        any = true;
        try {
            int line = std::stoi(token);
            breakpoints_.insert(line);
            out_ << "[DEBUG] breakpoint set at line " << line << "\n";
        } catch (...) {
            out_ << "[DEBUG] invalid line number: " << token << "\n";
        }
    }
    if (!any) out_ << "[DEBUG] usage: break <line> [line2 ...]\n";
}

void Debugger::cmdRemove(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    bool any = false;
    while (iss >> token) {
        any = true;
        try {
            int line = std::stoi(token);
            if (breakpoints_.erase(line))
                out_ << "[DEBUG] breakpoint removed at line " << line << "\n";
            else
                out_ << "[DEBUG] no breakpoint at line " << line << "\n";
        } catch (...) {
            out_ << "[DEBUG] invalid line number: " << token << "\n";
        }
    }
    if (!any) out_ << "[DEBUG] usage: remove <line> [line2 ...]\n";
}

void Debugger::cmdBreakpoints() {
    if (breakpoints_.empty()) {
        out_ << "[DEBUG] no breakpoints set\n";
        return;
    }
    out_ << "[DEBUG] breakpoints:";
    for (int line : breakpoints_) out_ << " " << line;
    out_ << "\n";
}

// ── watch commands ────────────────────────────────────────────────────
void Debugger::cmdWatch(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    bool any = false;
    while (iss >> token) {
        any = true;
        std::string name;
        int index = 0;
        if (parseIndexedArg(token, name, index)) {
            bool dup = false;
            for (const auto& w : indexedWatches_)
                if (w.name == name && w.index == index) { dup = true; break; }
            if (dup)
                out_ << "[WATCH] '" << token << "' is already watched\n";
            else {
                indexedWatches_.push_back({name, index});
                out_ << "[WATCH] now watching '" << token << "'\n";
            }
        } else {
            bool dup = false;
            for (const auto& w : watches_)
                if (w == token) { dup = true; break; }
            if (dup)
                out_ << "[WATCH] '" << token << "' is already watched\n";
            else {
                watches_.push_back(token);
                out_ << "[WATCH] now watching '" << token << "'\n";
            }
        }
    }
    if (!any) out_ << "[WATCH] usage: watch <name> [name2 ...]  or  watch <name>[<index>]\n";
}

void Debugger::cmdUnwatch(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    bool any = false;
    while (iss >> token) {
        any = true;
        std::string name;
        int index = 0;
        if (parseIndexedArg(token, name, index)) {
            bool found = false;
            for (auto it = indexedWatches_.begin(); it != indexedWatches_.end(); ++it) {
                if (it->name == name && it->index == index) {
                    indexedWatches_.erase(it);
                    out_ << "[WATCH] stopped watching '" << token << "'\n";
                    found = true;
                    break;
                }
            }
            if (!found) out_ << "[WATCH] '" << token << "' is not watched\n";
        } else {
            bool found = false;
            for (auto it = watches_.begin(); it != watches_.end(); ++it) {
                if (*it == token) {
                    watches_.erase(it);
                    out_ << "[WATCH] stopped watching '" << token << "'\n";
                    found = true;
                    break;
                }
            }
            if (!found) out_ << "[WATCH] '" << token << "' is not watched\n";
        }
    }
    if (!any) out_ << "[WATCH] usage: unwatch <name> [name2 ...]\n";
}

void Debugger::cmdWatches() {
    if (watches_.empty() && indexedWatches_.empty()) {
        out_ << "[WATCH] no watched variables\n";
        return;
    }
    for (const auto& name : watches_) {
        LiteralValue value;
        if (executor_.debugLookup(name, value))
            out_ << "[WATCH] " << name << " = " << stringifyValue(value) << "\n";
        else
            out_ << "[WATCH] " << name << " = <undefined>\n";
    }
    for (const auto& w : indexedWatches_)
        out_ << "[WATCH] " << resolveIndexedWatch(w.name, w.index) << "\n";
}

void Debugger::cmdInspect() {
    out_ << "--- current scope variables ---\n";
    auto vars = executor_.debugSnapshot();
    for (const auto& v : vars) {
        out_ << (v.isGlobal ? "[global] " : "[local] ")
             << v.name << " = " << stringifyValue(v.value)
             << " (" << typeName(v.value) << ")\n";
    }
}
