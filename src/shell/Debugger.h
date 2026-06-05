#pragma once
#include "src/executor/IStmtHook.h"
#include <istream>
#include <ostream>
#include <set>
#include <string>
#include <vector>

// ── Debugger — 디버그 모드 ────────────────────────────────────────
// 소스 코드를 Stmt 단위로 정지시키며 실행 상태를 점검한다.
// Executor 의 IStmtHook 을 구현해 각 Stmt 실행 직전에 개입한다.
// (Observer Pattern + Command Pattern: 명령 문자열 → 디버거 동작)
//
// 명령어
//   step          현재 Stmt 실행 후 다음 Stmt 에서 정지 (블록 내부 진입)
//   next          현재 Stmt 실행 (블록 내부로 진입하지 않음)
//   continue      다음 breakpoint 까지 실행
//   break <줄>    해당 줄에 breakpoint 설정
//   breakpoints   breakpoint 목록 출력
//   remove <줄>   breakpoint 해제
//   watch <변수>  감시 목록에 추가 (정지 시마다 자동 출력)
//   unwatch <변수> 감시 목록에서 제거
//   watches       감시 중인 변수 목록과 값 출력 (가장 인접한 스코프)
//   inspect       현재 스코프의 모든 변수와 값 출력
//   quit / exit   디버깅 종료
class Debugger : private IStmtHook {
public:
    // 소스 파일을 디버그 모드로 실행
    // cmdIn: 디버거 명령 입력 스트림, out: 프로그램 출력 + 디버거 메시지
    // 반환값: 0 = 정상 종료, 1 = 오류 (파일 없음 / 파이프라인 오류)
    int runFile(const std::string& path, std::istream& cmdIn, std::ostream& out);

    // 소스 문자열을 직접 디버그 실행 (테스트 용이)
    int runSource(const std::string& source, std::istream& cmdIn, std::ostream& out);

private:
    // 정지 해제 방식
    enum class Resume {
        Step,       // 다음 Stmt 에서 무조건 정지
        Next,       // 같은 깊이 이하의 다음 Stmt 에서 정지 (블록 내부 스킵)
        Continue,   // breakpoint 에서만 정지
    };

    std::set<int>            breakpoints_;
    std::vector<std::string> watches_;      // 등록 순서 유지
    std::vector<std::string> sourceLines_;  // 정지 위치의 소스 표시용
    Resume mode_       = Resume::Step;      // 시작 시 첫 Stmt 에서 정지
    int    pauseDepth_ = 0;                 // next 명령 시점의 깊이

    std::istream* cmdIn_ = nullptr;
    std::ostream* out_   = nullptr;

    // IStmtHook
    void onBeforeStmt(const Stmt& s, int depth, Environment& env) override;

    bool shouldPause(const Stmt& s, int depth) const;
    void printLocation(const Stmt& s) const;
    void printWatches(Environment& env) const;
    void printValue(const std::string& name, Environment& env) const;

    // 명령 루프: 실행 재개 명령(step/next/continue)이 나올 때까지 반복
    void commandLoop(int depth, Environment& env);
};
