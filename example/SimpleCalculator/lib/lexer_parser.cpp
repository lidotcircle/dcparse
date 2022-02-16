#include "scalc/lexer_parser.h"
using namespace std;


CalcLexerParser::CalcLexerParser()
{
}

void CalcLexerParser::feed(char c)
{
    auto tokens = this->lexer.feed(c);
    for (auto t: tokens)
        this->parser.feed(t);
}

std::shared_ptr<ASTNode> CalcLexerParser::end()
{
    auto tokens = this->lexer.end();
    for (auto t: tokens)
        this->parser.feed(t);

    auto nonterm = this->parser.end();
    auto calcunit = dynamic_pointer_cast<NonTermCalcUnit>(nonterm);
    return calcunit->astnode;
}

void CalcLexerParser::reset()
{
    this->lexer.reset();
    this->parser.reset();
}
