#ifndef _SIMPLE_CALCULATOR_PARSER_H_
#define _SIMPLE_CALCULATOR_PARSER_H_

#include "ast.h"
#include "parser/parser.h"


#define NONTERMS                                                                                   \
    TENTRY(CalcUnit)                                                                               \
    TENTRY(ArgList)                                                                                \
    TENTRY(FunctionDef)                                                                            \
    TENTRY(FunctionDecl)                                                                           \
                                                                                                   \
    TENTRY(IfStatement)                                                                            \
    TENTRY(ForStatement)                                                                           \
    TENTRY(WhileStatement)                                                                         \
    TENTRY(ReturnStatement)                                                                        \
    TENTRY(StatementList)                                                                          \
    TENTRY(BlockStatement)                                                                         \
    TENTRY(ExprStatement)                                                                          \
    TENTRY(Statement)                                                                              \
                                                                                                   \
    TENTRY(ExprList)                                                                               \
    TENTRY(Expr)

#define TENTRY(n)                                                                                  \
    struct NonTerm##n : public NonTerminal                                                         \
    {                                                                                              \
        std::shared_ptr<ASTNode> astnode;                                                          \
        inline NonTerm##n(std::shared_ptr<ASTNode> node) : astnode(node)                           \
        {}                                                                                         \
    };
NONTERMS
#undef TENTRY

// Forward declaration
class SCalcContext;

class SCalcParserContext : public DCParser::DCParserContext
{
  private:
    std::shared_ptr<SCalcContext> m_execution_context;
    bool m_execute;
    std::ostream* m_out;

  public:
    SCalcParserContext() = delete;
    SCalcParserContext(DCParser& parser, bool execute);

    inline bool executable() const
    {
        return this->m_execute;
    }
    inline std::ostream* output()
    {
        return this->m_out;
    }
    inline void set_output(std::ostream* out)
    {
        this->m_out = out;
    }

    std::shared_ptr<SCalcContext> ExecutionContext();
    const std::shared_ptr<SCalcContext> ExecutionContext() const;
};

class CalcParser : private DCParser
{
  private:
    void expression_rules();
    void statement_rules();
    void function_rules();
    void calcunit_rules();

  public:
    CalcParser(bool execute);

    using DCParser::end;
    using DCParser::feed;
    using DCParser::getContext;
    using DCParser::next_possible_token_of;
    using DCParser::parse;
    using DCParser::prev_possible_token_of;
    using DCParser::query_charinfo;
    using DCParser::reset;
    using DCParser::setDebugStream;
};

#endif // _SIMPLE_CALCULATOR_PARSER_H_
