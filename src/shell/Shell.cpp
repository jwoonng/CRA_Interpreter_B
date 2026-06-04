#include "Shell.h"
#include <sstream>

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
void Shell::processLine(const std::string& line, std::ostream& out) {
    if (line.empty()) return;
    try {
        auto tokens = tokenizer_.tokenize(line);
        auto stmts  = parser_.parse(tokens);
        checker_.check(stmts);
        executor_.execute(stmts, out);
    } catch (const std::exception& e) {
        out << e.what() << "\n";
    }
}
