#ifndef _SIMPLE_CALCULATOR_LEXER_PARSER_H_
#define _SIMPLE_CALCULATOR_LEXER_PARSER_H_

#include "./token.h"
#include "./parser.h"


class CalcLexerParser {
private:
    CalcParser parser;
    CalcLexer  lexer;

public:
    CalcLexerParser(bool execute);

    void feed(char c);
    std::shared_ptr<ASTNode> end();
    void reset();

    std::shared_ptr<SCalcParserContext> getContext();
    inline void setDebugStream(std::ostream& out) { this->parser.setDebugStream(out); }
};

#endif // _SIMPLE_CALCULATOR_LEXER_PARSER_H_
