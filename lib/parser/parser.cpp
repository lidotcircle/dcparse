#include "parser/parser.h"
#include "assert.h"
#include <map>
#include <stdexcept>
using namespace std;

using charid_t = DCParser::charid_t;
using ruleid_t = DCParser::ruleid_t;
using state_t  = DCParser::state_t;
using reduce_callback_t = DCParser::reduce_callback_t;

struct RuleOption {
    size_t            priority;
    RuleAssocitive    associtive;
};

struct PushdownEntry
{
private:
    enum PushdownType {
        STATE_TYPE_SHIFT,
        STATE_TYPE_REDUCE,
        STATE_TYPE_LOOKAHEAD,
        STATE_TYPE_REJECT,
    } _type;

    union MM {
        state_t state;
        ruleid_t rule;
        shared_ptr<PushdownStateMapping> next;

        MM(): state(0) {}
        ~MM() {}
    } _u;

    class StateTypeH {};
    class RuleTypeH {};

    PushdownEntry(): _type(STATE_TYPE_REJECT) { }
    PushdownEntry(state_t state, StateTypeH): _type(STATE_TYPE_SHIFT) { _u.state = state; }
    PushdownEntry(ruleid_t rule, RuleTypeH): _type(STATE_TYPE_REDUCE) { _u.rule = rule; }
    PushdownEntry(shared_ptr<PushdownStateMapping> next)
        : _type(STATE_TYPE_LOOKAHEAD)
    {
        new(&_u.next) shared_ptr<PushdownStateMapping>(next);
    }


public:
    PushdownType type() const { return this->_type; }

    PushdownEntry(const PushdownEntry& other)
        : _type(other._type)
    {
        switch (this->_type) {
        case STATE_TYPE_SHIFT:
            this->_u.state = other._u.state;
            break;
        case STATE_TYPE_REDUCE:
            this->_u.rule = other._u.rule;
            break;
        case STATE_TYPE_LOOKAHEAD:
            new(&this->_u.next) shared_ptr<PushdownStateMapping>(other._u.next);
            break;
        case STATE_TYPE_REJECT:
            break;
        }
    }

    PushdownEntry& operator=(const PushdownEntry& other)
    {
        if (this->type() == STATE_TYPE_LOOKAHEAD) {
            this->_u.next.~shared_ptr<PushdownStateMapping>();
        }

        this->_type = other._type;
        switch (this->_type) {
        case STATE_TYPE_SHIFT:
            this->_u.state = other._u.state;
            break;
        case STATE_TYPE_REDUCE:
            this->_u.rule = other._u.rule;
            break;
        case STATE_TYPE_LOOKAHEAD:
            new(&this->_u.next) shared_ptr<PushdownStateMapping>(other._u.next);
            break;
        case STATE_TYPE_REJECT:
            break;
        }

        return *this;
    }

    static PushdownEntry shift(state_t state) { return PushdownEntry(state, StateTypeH()); }
    static PushdownEntry reduce(ruleid_t rule) { return PushdownEntry(rule, RuleTypeH()); }
    static PushdownEntry lookahead(shared_ptr<PushdownStateMapping> next) { return PushdownEntry(next); }
    static PushdownEntry reject() { return PushdownEntry(); }

    ~PushdownEntry()
    {
        if (this->type() == STATE_TYPE_LOOKAHEAD) {
            this->_u.next.~shared_ptr<PushdownStateMapping>();
        }
    }
};

using  PushdownStateMappingTX = vector<map<charid_t, PushdownEntry>>;
struct PushdownStateMapping { PushdownStateMappingTX val; };

void DCParser::ensure_epsilon_closure()
{
    if (!this->u_epsilon_closure.empty())
        return;

    assert(this->u_possible_next.empty());

    std::map<charid_t,std::set<ruleid_t>> pvpv;
    for (size_t i=0;i<this->m_rules.size();i++) {
        auto& r = this->m_rules[i];
        pvpv[r.m_lhs].insert(i);
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& rs: pvpv) {
            auto cr = rs.first;
            auto& rx = rs.second;

            for (auto& r: rx) {
                assert(r < this->m_rules.size());
                auto& rr = this->m_rules[r];

                if (rr.m_rhs.size() > 0) {
                    auto cx = rr.m_rhs[0];

                    if (pvpv.find(cx) != pvpv.end()) {
                        rx.insert(pvpv[cx].begin(), pvpv[cx].end());
                        changed = true;
                    }
                }
            }
        }
    }

    for (auto& rs: pvpv) {
        auto cr = rs.first;
        auto& rx = rs.second;

        for (auto r: rx) {
            this->u_epsilon_closure[cr].push_back(r);
            assert(r < this->m_rules.size());
            auto& rr = this->m_rules[r];

            if (rr.m_rhs.size() > 0) {
                auto cx = rr.m_rhs[0];
                this->u_possible_next[cr].insert(cx);
            }
        }
    }
}

bool DCParser::is_nonterm(charid_t id) const {
    return this->m_nonterms.find(id) != this->m_nonterms.end();
}

set<pair<ruleid_t,size_t>> DCParser::startState() const {
    set<pair<ruleid_t,size_t>> ret;
    auto _this = const_cast<DCParser*>(this);
    _this->ensure_epsilon_closure();

    for (auto& start_sym: this->m_start_symbols) {
        auto& start_rules = _this->u_epsilon_closure[start_sym];
        for (auto& start_rule: start_rules) {
            ret.insert(make_pair(start_rule, 0));
        }
    }

    return ret;
}


DCParser::DCParser(): m_priority(0) { }

void DCParser::dec_priority() { this->m_priority++; }

int DCParser::add_rule(charid_t lh, vector<charid_t> rh,
                       reduce_callback_t cb,
                       RuleAssocitive associtive)
{
    this->m_nonterms.insert(lh);
    this->m_symbols.insert(lh);
    if (this->m_terms.find(lh) != this->m_terms.end())
        this->m_terms.erase(lh);

    for (auto r: rh) {
        this->m_symbols.insert(r);
        if (this->m_nonterms.find(r) != this->m_nonterms.end())
            this->m_terms.insert(r);
    }

    RuleInfo ri;
    ri.m_lhs = lh;
    ri.m_rhs = rh;
    ri.m_reduce_callback = cb;
    ri.m_rule_option = std::make_shared<RuleOption>();

    ri.m_rule_option->associtive = associtive;
    ri.m_rule_option->priority = this->m_priority;

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
    state_t allocated_state = 0;
    map<set<pair<ruleid_t,size_t>>,state_t> state_map;
    const auto query_state = [&](set<pair<ruleid_t,size_t>> s)
    {
        if (state_map.find(s) == state_map.end())
            state_map[s] = allocated_state++;

        return state_map[s];
    };

    this->m_pds_mapping = make_shared<PushdownStateMapping>();
    auto& mapping = this->m_pds_mapping->val;

    auto s_start_state = this->startState();
    auto start_state = query_state(s_start_state);
    if (mapping.size() <= start_state)
        mapping.resize(start_state+1);
}

