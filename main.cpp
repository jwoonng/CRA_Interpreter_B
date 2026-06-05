#include "src/shell/Shell.h"
#include "src/optimizer/StaticBindingOptimizer.h"
#include "src/optimizer/ConstantFoldingOptimizer.h"
#include <iostream>

int main() {
    Shell shell;
    shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
    shell.addOptimizer(std::make_unique<ConstantFoldingOptimizer>());
    shell.run(std::cin, std::cout);
    return 0;
}
