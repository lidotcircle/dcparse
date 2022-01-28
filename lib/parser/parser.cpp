#include "parser/parser.h"
#include <map>
#include <stdexcept>
using namespace std;

using charid_t = DCParser::charid_t;
using reduce_callback_t = DCParser::reduce_callback_t;

struct RuleOption {
    reduce_callback_t callback;
    int               priority;
    RuleAssocitive    associtive;
};

struct PushdownState {
    int state;
    int rule;
    shared_ptr<PushdownStateMapping> next;
};

struct PushdownStateMapping {
    map<DCParser::charid_t,PushdownState> mapping;
};

bool DCParser::is_nonterm(charid_t id) const {
    return this->m_nonterms.find(id) != this->m_nonterms.end();
}

set<pair<charid_t,charid_t>> DCParser::startState() const {
    set<pair<charid_t,charid_t>> ret;

    for (size_t i=0;i<this->m_rules.size();i++) {
        auto& rule = this->m_rules[i];
        auto nonterm = std::get<0>(rule);
        if (m_nonterms.find(nonterm) != m_nonterms.end()) {
            ret.insert(make_pair(i,0));
        }
    }

    return ret;
}

int DCParser::add_rule(charid_t lh, vector<charid_t> rh, reduce_callback_t cb,
                       int priority, RuleAssocitive associtive)
{
    /* FIXME
    auto option = make_unique<RuleOption>(cb, priority, associtive);
    this->m_rules.push_back(make_tuple(lh, rh, option));
    return this->m_rules.size();
    */
    return 0;
}

void DCParser::add_start_symbol(charid_t id) {
    if (this->m_start_symbols.find(id) != this->m_start_symbols.end()) {
        throw std::runtime_error("start symbol already exists");
    }

    if (!this->is_nonterm(id)) {
        throw std::runtime_error("start symbol is not a nonterminal");
    }

    this->m_start_symbols.insert(id);
}

void DCParser::generate_table()
{
    vector<charid_t> symbols;
    for (auto& nonterm: this->m_nonterms) {
        symbols.push_back(nonterm);
    }
    for (auto& term: this->m_terms) {
        symbols.push_back(term);
    }

    auto s =  this->startState();
    if (s.empty()) {
        throw std::runtime_error("no valid start symbol or "
                                 "no rules for start symbol");
    }
    vector<set<pair<charid_t,charid_t>>> states = { s };

    while (!states.empty()) {
    }
}

