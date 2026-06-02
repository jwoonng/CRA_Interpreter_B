#pragma once
#include "ITokenizer.h"

class Tokenizer : public ITokenizer {
public:
    std::vector<Token> tokenize(const std::string& source) override;
};
