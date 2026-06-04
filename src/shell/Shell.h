#pragma once
#include "src/assembler/Tokenizer.h"
#include "src/assembler/Parser.h"
#include "src/checker/Checker.h"
#include "src/executor/Executor.h"
#include <istream>
#include <ostream>
#include <string>

// ── Prompt REPL ───────────────────────────────────────────────────
// 한 줄씩 입력받아 Tokenizer → Parser → Checker → Executor 순으로 실행
// 변수 등 실행 상태는 라인 간에 유지된다
class Shell {
public:
    // 대화형 REPL: in 에서 한 줄씩 읽어 out 에 결과 출력
    void run(std::istream& in, std::ostream& out);

    // 테스트용: 한 줄을 실행하고 출력 결과를 문자열로 반환
    std::string runLine(const std::string& line);

private:
    Tokenizer tokenizer_;
    Parser    parser_;
    Checker   checker_;
    Executor  executor_;

    void processLine(const std::string& line, std::ostream& out);
};
