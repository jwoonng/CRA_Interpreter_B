#pragma once

// 전방 선언 — Executor.h 와의 순환 include 방지
struct Stmt;
struct Environment;

// ── IStmtHook — Stmt 실행 훅 (Observer Pattern) ───────────────────
// Executor 가 각 Stmt 를 실행하기 직전에 호출한다.
// 디버거(Stepping/Breakpoint/Watch)나 Test Double 이 구현해
// 실행 흐름을 관찰·정지시킬 수 있다.
struct IStmtHook {
    virtual ~IStmtHook() = default;

    // s     : 실행 직전의 Stmt
    // depth : 중첩 깊이 (최상위 Stmt = 0, 블록/함수 내부일수록 증가)
    // env   : 현재 스코프 환경 (watch/inspect 용 변수 조회)
    virtual void onBeforeStmt(const Stmt& s, int depth, Environment& env) = 0;
};
