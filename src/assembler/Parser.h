#pragma once
#include "IParser.h"

class Parser : public IParser {
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens) override;
};
