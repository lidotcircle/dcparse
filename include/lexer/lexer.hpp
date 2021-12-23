#ifndef _LEXER_LEXER_HPP_
#define _LEXER_LEXER_HPP_

#include <vector>
#include <optional>
#include <string>
#include "lexer_rule.hpp"
#include "lexer_error.h"


template<typename T>
class Lexer {
public:
    using CharType = T;

private:
    std::vector<std::unique_ptr<LexerRule<CharType>>> rules;
    size_t highest_priority_candidator = std::string::npos;
    std::string m_filename;

    void reset_rules() {
        highest_priority_candidator = std::string::npos;
        for (auto& rule: rules) {
            rule->reset(line_num, col_num, pos, this->m_filename);
        }
    }

protected:
    size_t line_num = 1, col_num = 0, pos = 0;
    virtual void update_position_info(CharType c) {
        pos += 1;

        if (c == '\n') {
            line_num++;
            col_num = 1;
        } else {
            col_num++;
        }
    }

public:
    Lexer() = default;
    Lexer(const std::string& fn): m_filename(fn) {}
    ~Lexer() = default;

    void add_rule(std::unique_ptr<LexerRule<CharType>> rule) {
        rules.push_back(std::move(rule));
    }

    std::optional<std::shared_ptr<LexerToken>> feed_char(CharType c) {
        this->update_position_info(c);
        if (highest_priority_candidator == std::string::npos)
            this->reset_rules();

        for (size_t i=0;i<rules.size();i++) {
            auto& rule = rules[i];
            if (rule->dead())
                continue;

            rule->feed(c);
            if (i == highest_priority_candidator && rule->dead()) {
                i = std::string::npos;
            } else if ((highest_priority_candidator == std::string::npos || 
                 i < highest_priority_candidator) && !rule->dead())
            {
                highest_priority_candidator = i;
            }
        }

        if (highest_priority_candidator == std::string::npos) {
            throw LexerError("no rule matched at " + std::to_string(line_num) + std::to_string(col_num));
        }

        auto& can = rules[highest_priority_candidator];
        if (can->match()) {
            this->reset_rules();
            return can->get_token();
        } else {
            return std::nullopt;
        }
    }
};

#endif // _LEXER_LEXER_HPP_
