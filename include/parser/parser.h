#ifndef _DC_PARSER_HPP_
#define _DC_PARSER_HPP_

#include "../lexer/token.h"
#include <vector>
#include <memory>
#include <tuple>
#include <set>
#include <map>
#include <functional>

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
    using ruleid_t = size_t;
    using state_t  = size_t;
    using reduce_callback_t = std::function<dnonterm_t(DCParser& parser, std::vector<dchar_t>& children)>;

private:
    struct RuleInfo {
        charid_t                    m_lhs;
        std::vector<charid_t>       m_rhs;
        reduce_callback_t           m_reduce_callback;
        std::shared_ptr<RuleOption> m_rule_option;
    };
    std::vector<RuleInfo> m_rules;
    std::set<charid_t> m_nonterms;
    std::set<charid_t> m_terms;
    std::set<charid_t> m_symbols;
    std::set<charid_t> m_start_symbols;
    size_t m_priority;

    std::shared_ptr<PushdownStateMapping> m_pds_mapping;

private:
    std::map<charid_t,std::vector<ruleid_t>> u_epsilon_closure;
    std::map<charid_t,std::set<charid_t>>    u_possible_next;
    void ensure_epsilon_closure();

    bool is_nonterm(charid_t id) const;
    std::set<std::pair<ruleid_t,size_t>> startState() const;

public:
    DCParser();
    virtual ~DCParser() = default;

    void dec_priority();

    int  add_rule(charid_t leftside, std::vector<charid_t> rightside,
                  reduce_callback_t reduce_cb, RuleAssocitive associative = RuleAssocitiveLeft);

    void add_start_symbol(charid_t start);

    void generate_table();

    void feed(dctoken_t token);
    void end();
};

#endif // _DC_PARSER_HPP_
