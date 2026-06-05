#pragma once
#include <istream>
#include <ostream>

// ── ShellLauncher — 공장 제어 쉘 진입점 ───────────────────────────
// 명령행 인자로 Interpreter Factory 실행 모드를 선택한다.
//
//   (인자 없음)          프롬프트 모드 (REPL)
//   <script>             파일 모드
//   --debug <script>     디버그 모드 (Stmt 단위 stepping)
//
// 반환값: 프로세스 종료 코드 (0 = 정상)
int launchShell(int argc, char* argv[], std::istream& in, std::ostream& out);
