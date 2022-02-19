#include "scalc/lexer_parser.h"
using namespace std;


CalcLexerParser::CalcLexerParser(bool execute):
    parser(execute)
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

std::shared_ptr<SCalcParserContext> CalcLexerParser::getContext()
{
    auto ctx = this->parser.getContext();
    auto rctx = dynamic_pointer_cast<SCalcParserContext>(ctx);
    assert(rctx);
    return rctx;
}
