#include "Debugger.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

// quit/exit 명령으로 실행을 중단할 때 사용하는 제어 흐름 예외
struct DebugQuit {};

std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// "break 5" → ("break", "5") 처럼 첫 단어와 나머지를 분리
std::pair<std::string, std::string> splitCommand(const std::string& line) {
    std::string t = trim(line);
    auto sp = t.find_first_of(" \t");
    if (sp == std::string::npos) return {t, ""};
    return {t.substr(0, sp), trim(t.substr(sp + 1))};
}

// 첫 단어를 소문자로 통일 (PDF 표기 'Breakpoints' 같은 대문자 허용)
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool parseLineNumber(const std::string& arg, int& outLine) {
    if (arg.empty()) return false;
    for (char c : arg)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    outLine = std::stoi(arg);
    return true;
}

// 가장 인접한 스코프부터 변수 탐색 (없으면 nullptr)
const LiteralValue* findVariable(Environment& env, const std::string& name) {
    for (Environment* e = &env; e; e = e->enclosing) {
        auto it = e->values.find(name);
        if (it != e->values.end()) return &it->second;
    }
    return nullptr;
}

} // namespace

// ── 공개 진입점 ───────────────────────────────────────────────────
int Debugger::runFile(const std::string& path, std::istream& cmdIn, std::ostream& out) {
    std::ifstream file(path);
    if (!file) {
        out << "Error: cannot open file '" << path << "'.\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return runSource(buf.str(), cmdIn, out);
}

int Debugger::runSource(const std::string& source, std::istream& cmdIn, std::ostream& out) {
    cmdIn_ = &cmdIn;
    out_   = &out;

    // 정지 위치 표시용 소스 라인 분리
    sourceLines_.clear();
    std::istringstream src(source);
    for (std::string line; std::getline(src, line); )
        sourceLines_.push_back(line);

    // 디버그 모드는 최적화 없이 소스 그대로 실행한다
    try {
        Tokenizer tokenizer;
        Parser    parser;
        Checker   checker;
        Executor  executor;

        auto tokens = tokenizer.tokenize(source);
        auto stmts  = parser.parse(tokens);
        checker.check(stmts);

        executor.setStmtHook(this);
        executor.execute(std::move(stmts), out);
    } catch (const DebugQuit&) {
        out << "[debug] session terminated.\n";
        return 0;
    } catch (const std::exception& e) {
        out << e.what() << "\n";
        return 1;
    }
    out << "[debug] program finished.\n";
    return 0;
}

// ── IStmtHook — Stmt 실행 직전 정지 판단 ──────────────────────────
void Debugger::onBeforeStmt(const Stmt& s, int depth, Environment& env) {
    if (!shouldPause(s, depth)) return;
    printLocation(s);
    printWatches(env);
    commandLoop(depth, env);
}

bool Debugger::shouldPause(const Stmt& s, int depth) const {
    switch (mode_) {
        case Resume::Step:     return true;
        case Resume::Next:     return depth <= pauseDepth_ || breakpoints_.count(s.line) > 0;
        case Resume::Continue: return breakpoints_.count(s.line) > 0;
    }
    return false;
}

// ── 출력 ──────────────────────────────────────────────────────────
void Debugger::printLocation(const Stmt& s) const {
    *out_ << "[debug] stopped at line " << s.line;
    if (s.line >= 1 && s.line <= static_cast<int>(sourceLines_.size()))
        *out_ << ": " << trim(sourceLines_[s.line - 1]);
    *out_ << "\n";
}

void Debugger::printWatches(Environment& env) const {
    for (const auto& name : watches_)
        printValue(name, env);
}

void Debugger::printValue(const std::string& name, Environment& env) const {
    if (const LiteralValue* v = findVariable(env, name))
        *out_ << "  watch " << name << " = " << Executor::stringify(*v) << "\n";
    else
        *out_ << "  watch " << name << " = <not defined>\n";
}

// ── 명령 루프 (Command Pattern) ───────────────────────────────────
void Debugger::commandLoop(int depth, Environment& env) {
    std::string line;
    while (true) {
        *out_ << "(debug) ";
        if (!std::getline(*cmdIn_, line))
            throw DebugQuit{};   // 명령 입력 EOF → 세션 종료

        auto [word, arg] = splitCommand(line);
        std::string cmd  = toLower(word);
        if (cmd.empty()) continue;

        // ── 실행 재개 명령 ────────────────────────────────────────
        if (cmd == "step")     { mode_ = Resume::Step; return; }
        if (cmd == "next")     { mode_ = Resume::Next; pauseDepth_ = depth; return; }
        if (cmd == "continue") { mode_ = Resume::Continue; return; }
        if (cmd == "quit" || cmd == "exit") throw DebugQuit{};

        // ── breakpoint 관리 ──────────────────────────────────────
        if (cmd == "break") {
            int n;
            if (!parseLineNumber(arg, n)) { *out_ << "usage: break <line>\n"; continue; }
            breakpoints_.insert(n);
            *out_ << "breakpoint set at line " << n << "\n";
            continue;
        }
        if (cmd == "remove") {
            int n;
            if (!parseLineNumber(arg, n)) { *out_ << "usage: remove <line>\n"; continue; }
            if (breakpoints_.erase(n))
                *out_ << "breakpoint removed at line " << n << "\n";
            else
                *out_ << "no breakpoint at line " << n << "\n";
            continue;
        }
        if (cmd == "breakpoints") {
            if (breakpoints_.empty()) { *out_ << "no breakpoints\n"; continue; }
            *out_ << "breakpoints:";
            for (int n : breakpoints_) *out_ << " " << n;
            *out_ << "\n";
            continue;
        }

        // ── watch 관리 ───────────────────────────────────────────
        if (cmd == "watch") {
            if (arg.empty()) { *out_ << "usage: watch <variable>\n"; continue; }
            if (std::find(watches_.begin(), watches_.end(), arg) == watches_.end())
                watches_.push_back(arg);
            *out_ << "watching '" << arg << "'\n";
            continue;
        }
        if (cmd == "unwatch") {
            if (arg.empty()) { *out_ << "usage: unwatch <variable>\n"; continue; }
            auto it = std::find(watches_.begin(), watches_.end(), arg);
            if (it != watches_.end()) {
                watches_.erase(it);
                *out_ << "unwatched '" << arg << "'\n";
            } else {
                *out_ << "'" << arg << "' is not being watched\n";
            }
            continue;
        }
        if (cmd == "watches") {
            if (watches_.empty()) { *out_ << "no watches\n"; continue; }
            for (const auto& name : watches_)
                printValue(name, env);
            continue;
        }

        // ── 현재 스코프 점검 ─────────────────────────────────────
        if (cmd == "inspect") {
            if (env.values.empty()) { *out_ << "(no variables in current scope)\n"; continue; }
            for (const auto& [name, val] : env.values)
                *out_ << "  " << name << " = " << Executor::stringify(val) << "\n";
            continue;
        }

        *out_ << "unknown command: " << word << "\n"
              << "commands: step next continue break <line> breakpoints remove <line> "
                 "watch <var> unwatch <var> watches inspect quit\n";
    }
}
