#ifndef _DC_PARSER_HPP_
#define _DC_PARSER_HPP_

#include <vector>
#include <memory>
#include <tuple>
#include <set>
#include "../lexer/token.h"

using dchar_t   = std::shared_ptr<DChar>;
using dctoken_t = std::shared_ptr<LexerToken>;

class NonTerminal : public DChar { };
using dnonterm_t = std::shared_ptr<NonTerminal>;

struct RuleOption;
enum RuleAssocitive {
    RuleAssocitiveLeft,
    RuleAssocitiveRight,
};
struct PushdownStateMapping;

class DCParser {
public:
    using charid_t = size_t;
    typedef dnonterm_t (*reduce_callback_t)(DCParser* parser, std::vector<dchar_t>& children);
    dctoken_t current_token;

private:
    std::vector<std::tuple<charid_t,std::vector<charid_t>,std::unique_ptr<RuleOption>>> m_rules;
    std::set<charid_t> m_nonterms;
    std::set<charid_t> m_terms;
    std::set<charid_t> m_start_symbols;
    std::shared_ptr<PushdownStateMapping> m_pds_mapping;

private:
    bool is_nonterm(charid_t id) const;
    std::set<std::pair<charid_t,charid_t>> startState() const;

public:
    int  add_rule(charid_t leftside, std::vector<charid_t> rightside,
                  reduce_callback_t reduce_cb, int priority = 0, RuleAssocitive associative = RuleAssocitiveLeft);

    void add_start_symbol(charid_t start);

    void generate_table();

    void feed(dctoken_t token);
    void end();
};

#endif // _DC_PARSER_HPP_
