#pragma once
#include "src/assembler/ITokenizer.h"
#include "src/assembler/IParser.h"
#include "src/checker/IChecker.h"
#include "src/executor/IExecutor.h"
#include <istream>
#include <memory>
#include <ostream>
#include <string>

// ── Prompt REPL ───────────────────────────────────────────────────
// 한 줄씩 입력받아 Tokenizer → Parser → Checker → Executor 순으로 실행
// 변수 등 실행 상태는 라인 간에 유지된다
class Shell {
public:
    // 기본 생성자: 구체 구현(Tokenizer, Parser, Checker, Executor) 사용
    Shell();

    // 의존성 주입 생성자: 테스트에서 Mock 주입 가능
    Shell(std::unique_ptr<ITokenizer> tokenizer,
          std::unique_ptr<IParser>    parser,
          std::unique_ptr<IChecker>   checker,
          std::unique_ptr<IExecutor>  executor);

    // 대화형 REPL: in 에서 한 줄씩 읽어 out 에 결과 출력
    void run(std::istream& in, std::ostream& out);

    // 테스트용: 한 줄을 실행하고 출력 결과를 문자열로 반환
    std::string runLine(const std::string& line);

private:
    std::unique_ptr<ITokenizer> tokenizer_;
    std::unique_ptr<IParser>    parser_;
    std::unique_ptr<IChecker>   checker_;
    std::unique_ptr<IExecutor>  executor_;

    void processLine(const std::string& line, std::ostream& out);
};
