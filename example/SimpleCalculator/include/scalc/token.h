#ifndef _SIMPLE_CALCULATOR_TOKEN_H_
#define _SIMPLE_CALCULATOR_TOKEN_H_

#include "lexer/token.h"
#include "lexer/simple_lexer.hpp"
#include "dcutf8.h"
#include <string>


struct TokenID: public LexerToken {
    std::string id;

    inline TokenID(std::string id, LexerToken::TokenInfo info):
        id(id), LexerToken(info) {}
};

struct TokenNUMBER: public LexerToken {
    double num;

    inline TokenNUMBER(double val, LexerToken::TokenInfo info):
        num(val), LexerToken(info) {}
};

#define NTOKENS \
    TENTRY(PLUS) \
    TENTRY(MINUS) \
    TENTRY(MULTIPLY) \
    TENTRY(DIVISION) \
    TENTRY(REMAINDER) \
    TENTRY(CARET) \
    TENTRY(PLUSPLUS) \
    TENTRY(MINUSMINUS) \
    \
    TENTRY(EQUAL) \
    TENTRY(NOTEQUAL) \
    TENTRY(GREATERTHAN) \
    TENTRY(LESSTHAN) \
    TENTRY(GREATEREQUAL) \
    TENTRY(LESSEQUAL) \
    \
    TENTRY(ASSIGNMENT) \
    \
    TENTRY(LPAREN) \
    TENTRY(RPAREN) \
    TENTRY(LBRACE) \
    TENTRY(RBRACE) \
    \
    TENTRY(IF) \
    TENTRY(ELSE) \
    TENTRY(FOR) \
    TENTRY(WHILE) \
    TENTRY(FUNCTION) \
    TENTRY(RETURN) \
    \
    TENTRY(COMMA) \
    TENTRY(SEMICOLON)

#define TENTRY(n) \
    struct Token##n: public LexerToken { \
        inline Token##n(LexerToken::TokenInfo info): \
            LexerToken(info) {} \
    };
NTOKENS
#undef TENTRY


class CalcLexer: private Lexer<int>
{
public:
    using token_t = std::shared_ptr<LexerToken>;

private:
    UTF8Decoder m_decoder;

public:
    CalcLexer();

    std::vector<token_t> feed(char c);
    std::vector<token_t> end();

    void reset();
};

#endif // _SIMPLE_CALCULATOR_TOKEN_H_
