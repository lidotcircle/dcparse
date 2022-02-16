#ifndef _SIMPLE_CALCULATOR_LEXER_PARSER_H_
#define _SIMPLE_CALCULATOR_LEXER_PARSER_H_

#include "./token.h"
#include "./parser.h"


class CalcLexerParser {
private:
    CalcParser parser;
    CalcLexer  lexer;

public:
    CalcLexerParser();

    void feed(char c);
    std::shared_ptr<ASTNode> end();
    void reset();
};

#endif // _SIMPLE_CALCULATOR_LEXER_PARSER_H_
