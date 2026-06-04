#pragma once
#include "src/assembler/ITokenizer.h"
#include "src/assembler/IParser.h"
#include "src/checker/IChecker.h"
#include "src/executor/IExecutor.h"
#include "src/optimizer/IOptimizer.h"
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

// ── Shell — Prompt REPL ───────────────────────────────────────────
// 파이프라인: Tokenizer → Parser → Checker → [Optimizer*] → Executor
//
// Optimizer 는 addOptimizer() 로 체인에 추가한다 (Chain of Responsibility).
// 기본 생성자는 구체 구현을 사용하며, 의존성 주입 생성자로 Mock 교체 가능.
class Shell {
public:
    // 기본 생성자: 구체 구현(Tokenizer, Parser, Checker, Executor) 사용
    Shell();

    // 의존성 주입 생성자: 테스트 또는 커스텀 구현 주입 가능
    Shell(std::unique_ptr<ITokenizer> tokenizer,
          std::unique_ptr<IParser>    parser,
          std::unique_ptr<IChecker>   checker,
          std::unique_ptr<IExecutor>  executor);

    // Optimizer 체인에 패스 추가 (호출 순서대로 실행)
    void addOptimizer(std::unique_ptr<IOptimizer> optimizer);

    // 대화형 REPL: in 에서 한 줄씩 읽어 out 에 결과 출력
    void run(std::istream& in, std::ostream& out);

    // 테스트용: 한 줄을 실행하고 출력 결과를 문자열로 반환
    std::string runLine(const std::string& line);

private:
    std::unique_ptr<ITokenizer> tokenizer_;
    std::unique_ptr<IParser>    parser_;
    std::unique_ptr<IChecker>   checker_;
    std::vector<std::unique_ptr<IOptimizer>> optimizers_;  // 확장 가능한 최적화 체인
    std::unique_ptr<IExecutor>  executor_;

    void processLine(const std::string& line, std::ostream& out);
};
