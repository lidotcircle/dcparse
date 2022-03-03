#ifndef _C_PARSER_TOKEN_H_
#define _C_PARSER_TOKEN_H_

#include "lexer/token.h"
#include "lexer/simple_lexer.hpp"
#include "dcutf8.h"
#include <string>


namespace cparser {

struct TokenID: public LexerToken {
    std::string id;

    inline TokenID(std::string id, LexerToken::TokenInfo info):
        id(id), LexerToken(info) {}
};

struct TokenConstantInteger: public LexerToken {
    unsigned long long value;
    bool is_unsigned;

    inline TokenConstantInteger(unsigned long long val, LexerToken::TokenInfo info):
        is_unsigned(false), value(val), LexerToken(info) {}
};

struct TokenConstantFloat: public LexerToken {
    long double value;

    inline TokenConstantFloat(long double val, LexerToken::TokenInfo info):
        value(val), LexerToken(info) {}
};

struct TokenStringLiteral: public LexerToken {
    std::string value;

    inline TokenStringLiteral(std::string val, LexerToken::TokenInfo info):
        value(std::move(val)), LexerToken(info) {}
};

#define C_KEYWORD_LIST \
    K_ENTRY(auto) \
    K_ENTRY(break) \
    K_ENTRY(case) \
    K_ENTRY(char) \
    K_ENTRY(const) \
    K_ENTRY(continue) \
    K_ENTRY(default) \
    K_ENTRY(do) \
    K_ENTRY(double) \
    K_ENTRY(else) \
    K_ENTRY(enum) \
    K_ENTRY(extern) \
    K_ENTRY(float) \
    K_ENTRY(for) \
    K_ENTRY(goto) \
    K_ENTRY(if) \
    K_ENTRY(inline) \
    K_ENTRY(int) \
    K_ENTRY(long) \
    K_ENTRY(register) \
    K_ENTRY(restrict) \
    K_ENTRY(return) \
    K_ENTRY(short) \
    K_ENTRY(signed) \
    K_ENTRY(sizeof) \
    K_ENTRY(static) \
    K_ENTRY(struct) \
    K_ENTRY(switch) \
    K_ENTRY(typedef) \
    K_ENTRY(union) \
    K_ENTRY(unsigned) \
    K_ENTRY(void) \
    K_ENTRY(volatile) \
    K_ENTRY(while) \
    K_ENTRY(_Bool) \
    K_ENTRY(_Complex) \
    K_ENTRY(_Imaginary)

#define K_ENTRY(n) \
    struct TokenKeyword_##n: public LexerToken { \
        inline TokenKeyword_##n(LexerToken::TokenInfo info): \
            LexerToken(info) {} \
    };
C_KEYWORD_LIST
#undef K_ENTRY


#define C_PUNCTUATOR_LIST \
    P_ENTRY(LBRACKET, "\\[|<:") \
    P_ENTRY(RBRACKET, "\\]|:>") \
    P_ENTRY(LPAREN, "\\(") \
    P_ENTRY(RPAREN, "\\)") \
    P_ENTRY(LBRACE, "\\{|<%") \
    P_ENTRY(RBRACE, "\\}|%>") \
    P_ENTRY(DOT, "\\.") \
    P_ENTRY(PTRACCESS, "\\->") \
    P_ENTRY(PLUSPLUS, "\\+\\+") \
    P_ENTRY(MINUSMINUS, "\\-\\-") \
    P_ENTRY(REF, "&") \
    P_ENTRY(MULTIPLY, "\\*") \
    P_ENTRY(PLUS, "\\+") \
    P_ENTRY(MINUS, "\\-") \
    P_ENTRY(BIT_NOT, "~") \
    P_ENTRY(LOGIC_NOT, "!") \
    P_ENTRY(DIVISION, "/") \
    P_ENTRY(REMAIDER, "%") \
    P_ENTRY(LEFT_SHIFT, "<<") \
    P_ENTRY(RIGHT_SHIFT, ">>") \
    P_ENTRY(LESS_THAN, "<") \
    P_ENTRY(GREATER_THAN, ">") \
    P_ENTRY(LESS_EQUAL, "<=") \
    P_ENTRY(GREATER_EQUAL, ">=") \
    P_ENTRY(EQUAL, "==") \
    P_ENTRY(NOT_EQUAL, "!=") \
    P_ENTRY(BIT_XOR, "\\^") \
    P_ENTRY(BIT_OR, "\\|") \
    P_ENTRY(LOGIC_AND, "&&") \
    P_ENTRY(LOGIC_OR, "\\|\\|") \
    P_ENTRY(QUESTION, "\\?") \
    P_ENTRY(COLON, ":") \
    P_ENTRY(SEMICOLON, ";") \
    P_ENTRY(DOTS, "...") \
    P_ENTRY(ASSIGN, "=") \
    P_ENTRY(MULTIPLY_ASSIGN, "\\*=") \
    P_ENTRY(DIVISION_ASSIGN, "/=") \
    P_ENTRY(REMAIDER_ASSIGN, "%=") \
    P_ENTRY(PLUS_ASSIGN, "\\+=") \
    P_ENTRY(MINUS_ASSIGN, "\\-=") \
    P_ENTRY(LEFT_SHIFT_ASSIGN, "<<=") \
    P_ENTRY(RIGHT_SHIFT_ASSIGN, ">>=") \
    P_ENTRY(BIT_AND_ASSIGN, "&=") \
    P_ENTRY(BIT_OR_ASSIGN, "\\|=") \
    P_ENTRY(BIT_XOR_ASSIGN, "\\^=") \
    P_ENTRY(COMMA, ",") \
    P_ENTRY(SHARP, "#|%:") \
    P_ENTRY(DOUBLE_SHARP, "##|%:%:")

#define P_ENTRY(n, regex) \
    struct TokenPunc##n: public LexerToken { \
        inline TokenPunc##n(LexerToken::TokenInfo info): \
            LexerToken(info) {} \
    };
C_PUNCTUATOR_LIST
#undef P_ENTRY


class CLexer: private Lexer<int>
{
public:
    using token_t = std::shared_ptr<LexerToken>;

public:
    CLexer();

    std::vector<token_t> feed(int c);
    std::vector<token_t> end();

    void reset();
};

class CLexerUTF8: private CLexer
{
public:
    using token_t = std::shared_ptr<LexerToken>;

private:
    UTF8Decoder m_decoder;

public:
    CLexerUTF8();

    std::vector<token_t> feed(char c);
    using CLexer::end;
    using CLexer::reset;
};

}
#endif // _C_PARSER_TOKEN_H_
