#include "scalc/lexer_parser.h"
#include "scalc/llvm_visitor.h"
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
    m_calunit = calcunit->astnode;
    return calcunit->astnode;
}

void CalcLexerParser::reset()
{
    this->lexer.reset();
    this->parser.reset();
}

std::string CalcLexerParser::genllvm(const std::string& modulename) const
{
    assert(m_calunit);
    ASTNodeVisitorLLVMGen visitor(modulename);
    m_calunit->accept(visitor);
    return visitor.codegen();
}

std::shared_ptr<SCalcParserContext> CalcLexerParser::getContext()
{
    auto ctx = this->parser.getContext();
    auto rctx = dynamic_pointer_cast<SCalcParserContext>(ctx);
    assert(rctx);
    return rctx;
}
