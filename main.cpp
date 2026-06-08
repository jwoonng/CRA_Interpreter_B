#include "src/shell/Shell.h"
#include "src/optimizer/StaticBindingOptimizer.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
#include <iostream>
#include <string>

// ── Factory control shell entry point ─────────────────────────────────
// Usage:
//   factory                 interactive prompt mode (REPL)
//   factory run <path>      run a source file (file mode)
//   factory debug <path>    step through a source file (debug mode)
namespace {

void installOptimizers(Shell& shell) {
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
}

void printUsage(std::ostream& out) {
    out << "Usage:\n"
        << "  factory                 Start interactive prompt mode (REPL)\n"
        << "  factory run <path>      Execute a source file\n"
        << "  factory debug <path>    Step through a source file (debug mode)\n";
}

}  // namespace

int main(int argc, char** argv) {
    Shell shell;
    installOptimizers(shell);

    if (argc >= 2) {
        std::string mode = argv[1];

        if (mode == "run") {
            if (argc < 3) {
                std::cerr << "Error: 'run' requires a file path.\n";
                printUsage(std::cerr);
                return 1;
            }
            return shell.runFile(argv[2], std::cout);
        }

        if (mode == "debug") {
            if (argc < 3) {
                std::cerr << "Error: 'debug' requires a file path.\n";
                printUsage(std::cerr);
                return 1;
            }
            return shell.runDebug(argv[2], std::cin, std::cout);
        }

        if (mode == "-h" || mode == "--help") {
            printUsage(std::cout);
            return 0;
        }

        std::cerr << "Error: unknown mode '" << mode << "'.\n";
        printUsage(std::cerr);
        return 1;
    }

    // No arguments → interactive prompt (REPL) mode.
    shell.run(std::cin, std::cout);
    return 0;
}
