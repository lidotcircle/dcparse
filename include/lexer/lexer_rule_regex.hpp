#ifndef _LEXER_LEXER_RULE_REGEX_HPP_
#define _LEXER_LEXER_RULE_REGEX_HPP_

#include "./lexer_rule.hpp"
#include "../regex/regex.hpp"
#include <functional>
#include <vector>


template<typename T>
class LexerRuleRegex: public LexerRule<T> {
private:
    using CharType = T;
    using string_t = std::vector<CharType>;
    using token_factory_t = std::function<std::shared_ptr<LexerToken>(
            const std::vector<CharType>& token_value,
            size_t line, size_t column, 
            size_t pos, const std::string& filename)>;
    size_t m_line, m_column, m_pos;
    std::string m_filename;
    string_t m_string;
    SimpleRegExp<CharType> m_regex;
    token_factory_t m_token_factory;

public:
    LexerRuleRegex() = delete;
    template<typename Iterator>
    LexerRuleRegex(Iterator begin, Iterator end, token_factory_t factory): 
        m_regex(begin, end), m_token_factory(factory) {}

    LexerRuleRegex(const std::basic_string<CharType>& regex, token_factory_t factory): 
        m_regex(std::vector<CharType>(regex.begin(), regex.end())), m_token_factory(factory) {}

    virtual void feed(CharType c) override {
        this->m_regex.feed(c);
        if (!this->dead())
            this->m_string.push_back(c);
    }
    virtual bool dead() override {
        return this->m_regex.dead();
    }
    virtual bool match() override {
        return this->m_regex.match();
    };

    virtual void reset(size_t ln, size_t cn, size_t pos, const std::string& fn) override {
        this->m_line = ln;
        this->m_column = cn;
        this->m_pos = pos;
        this->m_filename = fn;
        this->m_regex.reset();
        this->m_string.clear();
    }

    virtual std::shared_ptr<LexerToken> token(std::vector<CharType> str) override {
        return this->m_token_factory(
                this->m_string,
                this->m_line, this->m_column, this->m_pos, this->m_filename);
    }
};

#endif // _LEXER_LEXER_RULE_REGEX_HPP_
