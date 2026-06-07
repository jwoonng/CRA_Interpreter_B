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

// ── 공개 인터페이스 ───────────────────────────────────────────────
// { } 깊이가 0이 될 때까지 줄을 누적한 뒤 한 번에 파이프라인에 넘긴다.
// 멀티라인 입력 중에는 "... " 프롬프트를 표시한다.
void Shell::run(std::istream& in, std::ostream& out) {
    std::string line;
    std::string accumulated;
    int depth = 0;

    out << "> ";
    while (std::getline(in, line)) {
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
