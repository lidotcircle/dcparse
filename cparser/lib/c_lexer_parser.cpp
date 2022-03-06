#include "c_lexer_parser.h"
using namespace std;
using namespace cparser;


CLexerParser::CLexerParser(): parser(), lexer() {}

void CLexerParser::feed(char c)
{
    auto ts = lexer.feed(c);
    for (auto& t: ts)
        parser.feed(t);
}

shared_ptr<ASTNodeTranslationUnit> CLexerParser::end() 
{
    return parser.end();
}

void CLexerParser::reset()
{
    this->lexer.reset();
    this->parser.reset();
}
