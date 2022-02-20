#ifndef _LEXER_LEXER_RULE_HPP_
#define _LEXER_LEXER_RULE_HPP_

#include "token.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>


template<typename T>
class LexerRule {
public:
    using CharType = T;
    LexerRule() = default;

    virtual void feed(CharType c) = 0;
    virtual bool dead() = 0;
    virtual bool match() = 0;

    virtual void reset(size_t ln, size_t cn, size_t pos, const std::string& fn,
                       std::optional<std::shared_ptr<LexerToken>> last_token) = 0;
    virtual std::shared_ptr<LexerToken> token(std::vector<CharType> str) = 0;

    virtual ~LexerRule() = default;
};

#endif // _LEXER_LEXER_RULE_HPP_
