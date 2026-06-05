#include "ShellLauncher.h"
#include "Debugger.h"
#include "Shell.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
#include "src/optimizer/StaticBindingOptimizer.h"
#include <string>
#include <vector>

namespace {

// REPL / 파일 모드용 Shell 생성 — 실행 전 최적화 체인 포함
Shell makeOptimizedShell() {
    Shell shell;
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    return shell;
}

void printUsage(std::ostream& out) {
    out << "Usage:\n"
        << "  (no args)            REPL mode    - interactive prompt ('exit'/'quit' to leave)\n"
        << "  <script>             File mode    - run a source file\n"
        << "  --debug <script>     Debug mode   - step through statements\n";
}

} // namespace

int launchShell(int argc, char* argv[], std::istream& in, std::ostream& out) {
    std::vector<std::string> args(argv + 1, argv + argc);

    // ── 프롬프트 모드 (REPL) ─────────────────────────────────────
    if (args.empty()) {
        Shell shell = makeOptimizedShell();
        shell.run(in, out);
        return 0;
    }

    // ── 디버그 모드 ──────────────────────────────────────────────
    if (args[0] == "--debug") {
        if (args.size() != 2) {
            out << "Error: --debug requires a script file.\n";
            printUsage(out);
            return 1;
        }
        Debugger debugger;
        return debugger.runFile(args[1], in, out);
    }

    // ── 알 수 없는 옵션 ──────────────────────────────────────────
    if (args[0].rfind("--", 0) == 0) {
        out << "Error: unknown option '" << args[0] << "'.\n";
        printUsage(out);
        return 1;
    }

    // ── 파일 모드 ────────────────────────────────────────────────
    if (args.size() != 1) {
        out << "Error: too many arguments.\n";
        printUsage(out);
        return 1;
    }
    Shell shell = makeOptimizedShell();
    return shell.runFile(args[0], out);
}
