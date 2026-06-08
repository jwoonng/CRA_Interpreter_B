#include "Shell.h"
#include "Debugger.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include <fstream>
#include <sstream>
#include <vector>

// ── 생성자 ────────────────────────────────────────────────────────
Shell::Shell()
    : tokenizer_(std::make_unique<Tokenizer>())
    , parser_(std::make_unique<Parser>())
    , checker_(std::make_unique<Checker>())
    , executor_(std::make_unique<Executor>())
{}

Shell::Shell(std::unique_ptr<ITokenizer> tokenizer,
             std::unique_ptr<IParser>    parser,
             std::unique_ptr<IChecker>   checker,
             std::unique_ptr<IExecutor>  executor)
    : tokenizer_(std::move(tokenizer))
    , parser_(std::move(parser))
    , checker_(std::move(checker))
    , executor_(std::move(executor))
{}

// ── Optimizer 체인 관리 ───────────────────────────────────────────
void Shell::addOptimizer(std::unique_ptr<IOptimizer> optimizer) {
    optimizers_.push_back(std::move(optimizer));
}

// ── 중괄호 깊이 계산 ─────────────────────────────────────────────
// 문자열 리터럴("...") 과 줄 주석(//) 안의 { }는 무시한다.
static int netBraces(const std::string& line) {
    int depth = 0;
    bool inString = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inString) {
            if (c == '\\' && i + 1 < line.size()) { ++i; continue; }
            if (c == '"') inString = false;
        } else {
            if      (c == '"')                                         inString = true;
            else if (c == '/' && i + 1 < line.size() && line[i+1] == '/') break;
            else if (c == '{') ++depth;
            else if (c == '}') --depth;
        }
    }
    return depth;
}

// ── trim helper ───────────────────────────────────────────────────
static std::string trimmed(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// ── prompt mode (REPL) ────────────────────────────────────────────
// Accumulate lines until the { } depth returns to 0, then run the whole
// block at once. While inside a multi-line block the "... " prompt is shown.
// Typing "exit" or "quit" at the top level ends the session.
void Shell::run(std::istream& in, std::ostream& out) {
    std::string line;
    std::string accumulated;
    int depth = 0;

    out << "> ";
    while (std::getline(in, line)) {
        // "exit" / "quit" terminate the session, but only at the top level
        // (not while accumulating a multi-line block).
        if (accumulated.empty() && depth == 0) {
            std::string t = trimmed(line);
            if (t == "exit" || t == "quit") break;
        }

        depth += netBraces(line);
        if (!accumulated.empty()) accumulated += '\n';
        accumulated += line;

        if (depth <= 0) {
            depth = 0;
            processLine(accumulated, out);
            accumulated.clear();
            out << "> ";
        } else {
            out << "... " << std::string((depth - 1) * 2, ' ');
        }
    }
    // EOF 도달 시 미완성 입력 처리
    if (!accumulated.empty())
        processLine(accumulated, out);
}

std::string Shell::runLine(const std::string& line) {
    std::ostringstream oss;
    processLine(line, oss);
    return oss.str();
}

// ── file reading helper ───────────────────────────────────────────
// On success fills source and returns true; returns false on open failure.
static bool readSourceFile(const std::string& path, std::string& source) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream buf;
    buf << file.rdbuf();
    source = buf.str();
    return true;
}

// ── file mode ─────────────────────────────────────────────────────
int Shell::runFile(const std::string& path, std::ostream& out) {
    std::string source;
    if (!readSourceFile(path, source)) {
        out << "Error: cannot open file '" << path << "'\n";
        return 1;
    }
    try {
        auto tokens = tokenizer_->tokenize(source);
        auto stmts  = parser_->parse(tokens);
        checker_->check(stmts);
        for (auto& opt : optimizers_)
            stmts = opt->optimize(std::move(stmts));
        executor_->execute(std::move(stmts), out);
    } catch (const std::exception& e) {
        // Runtime/parse error messages use the "[line N] ..." format — stop now.
        out << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ── debug mode ────────────────────────────────────────────────────
int Shell::runDebug(const std::string& path, std::istream& cmdIn, std::ostream& out) {
    std::string source;
    if (!readSourceFile(path, source)) {
        out << "Error: cannot open file '" << path << "'\n";
        return 1;
    }

    // Split into lines so the debugger can show source by line (index = line - 1).
    std::vector<std::string> lines;
    {
        std::istringstream ss(source);
        std::string ln;
        while (std::getline(ss, ln)) lines.push_back(ln);
    }

    try {
        auto tokens = tokenizer_->tokenize(source);
        auto stmts  = parser_->parse(tokens);
        checker_->check(stmts);
        for (auto& opt : optimizers_)
            stmts = opt->optimize(std::move(stmts));

        out << "[DEBUG] loaded source: " << path << "\n";
        // Debug stepping is driven through the IExecutor interface — the
        // built-in Executor overrides the hooks; other executors no-op.
        Debugger debugger(*executor_, std::move(lines), cmdIn, out);
        executor_->setDebugObserver(&debugger);
        try {
            executor_->execute(std::move(stmts), out);
        } catch (...) {
            executor_->setDebugObserver(nullptr);
            throw;
        }
        executor_->setDebugObserver(nullptr);
        out << "[DEBUG] execution finished\n";
    } catch (const std::exception& e) {
        out << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ── 파이프라인 ────────────────────────────────────────────────────
// Tokenizer → Parser → Checker → [Optimizer*] → Executor
void Shell::processLine(const std::string& line, std::ostream& out) {
    if (line.empty()) return;
    try {
        auto tokens = tokenizer_->tokenize(line);
        auto stmts  = parser_->parse(tokens);
        checker_->check(stmts);
        for (auto& opt : optimizers_)
            stmts = opt->optimize(std::move(stmts));
        executor_->execute(std::move(stmts), out);
    } catch (const std::exception& e) {
        out << e.what() << "\n";
    }
}
