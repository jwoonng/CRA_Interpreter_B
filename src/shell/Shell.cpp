#include "Shell.h"
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include <fstream>
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

// ── 내부 헬퍼 ────────────────────────────────────────────────────
namespace {
std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
} // namespace

// ── 공개 인터페이스 ───────────────────────────────────────────────
void Shell::run(std::istream& in, std::ostream& out) {
    std::string line;
    out << "> ";
    while (std::getline(in, line)) {
        std::string cmd = trim(line);
        if (cmd == "exit" || cmd == "quit") return;   // 세션 종료
        processLine(line, out);
        out << "> ";
    }
}

// 파일 모드: 파일 전체를 읽어 한 번에 실행
int Shell::runFile(const std::string& path, std::ostream& out) {
    std::ifstream file(path);
    if (!file) {
        out << "Error: cannot open file '" << path << "'.\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return runSource(buf.str(), out);
}

// 소스 전체 실행 — 오류 발생 시 메시지(줄 번호 포함) 출력 후 즉시 종료
int Shell::runSource(const std::string& source, std::ostream& out) {
    try {
        auto tokens = tokenizer_->tokenize(source);
        auto stmts  = parser_->parse(tokens);
        checker_->check(stmts);
        for (auto& opt : optimizers_)
            stmts = opt->optimize(std::move(stmts));
        executor_->execute(std::move(stmts), out);
    } catch (const std::exception& e) {
        out << e.what() << "\n";
        return 1;
    }
    return 0;
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
