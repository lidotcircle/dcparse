#ifndef _LEXER_SIMPLE_LEXER_HPP_
#define _LEXER_SIMPLE_LEXER_HPP_

#include "lexer.hpp"
#include "lexer_error.h"
#include <string>
#include <memory>
#include <vector>


template<typename T>
class SimpleLexer {
public:
    using CharType = T;
    using token_t = std::shared_ptr<LexerToken>;

private:
    std::unique_ptr<Lexer<CharType>> lexer;
    std::vector<token_t> token_buffer;
    size_t cur_pos;
    std::vector<CharType> buffer;
    size_t buf_pos;

    void fill_one() {
        if (cur_pos != std::string::npos || buf_pos >= buffer.size())
            return;

        while(buf_pos<buffer.size()) {
            auto c = buffer[buf_pos];
            buf_pos++;

            auto token = lexer->feed_char(c);
            if (token) {
                token_buffer.insert(token_buffer.begin(), token);
                if (token_buffer.size() > 10)
                    token_buffer.pop_back();
                cur_pos = 0;
                break;
            }
        }
    }

public:
    SimpleLexer(std::unique_ptr<Lexer<CharType>> lexer, std::vector<CharType> buf):
        lexer(std::move(lexer)), buffer(std::move(buf)), buf_pos(0), cur_pos(std::string::npos) {}

    bool end() const {
        auto _this = const_cast<SimpleLexer*>(this);
        _this->fill_one();
        return cur_pos == std::string::npos;
    }

    token_t next() {
        if (this->end())
            throw LexerError("No more tokens");

        auto token = token_buffer[cur_pos];
        if (cur_pos == 0)
            cur_pos = std::string::npos;
        else
            cur_pos--;

        return token;
    }

    void back() {
        if (this->cur_pos >= this->token_buffer.size() || this->cur_pos == std::string::npos)
            throw LexerError("lexer can't go backward");

        this->cur_pos++;
    }
};

#endif // _LEXER_SIMPLE_LEXER_HPP_
