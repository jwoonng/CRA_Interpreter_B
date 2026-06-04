#include "Shell.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include <sstream>

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

// ── 공개 인터페이스 ───────────────────────────────────────────────
void Shell::run(std::istream& in, std::ostream& out) {
    std::string line;
    out << "> ";
    while (std::getline(in, line)) {
        processLine(line, out);
        out << "> ";
    }
}

std::string Shell::runLine(const std::string& line) {
    std::ostringstream oss;
    processLine(line, oss);
    return oss.str();
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
        executor_->execute(stmts, out);
    } catch (const std::exception& e) {
        out << e.what() << "\n";
    }
}
