#ifndef _DC_PARSER_REGEX_HPP_
#define _DC_PARSER_REGEX_HPP_

#include <limits>
#include <algorithm>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <assert.h>
#include "./misc.hpp"


template<typename CharT>
struct character_traits {
    using char_type = CharT;

    constexpr static char_type LPAREN    = '(';
    constexpr static char_type RPAREN    = ')';
    constexpr static char_type LBRACKET  = '[';
    constexpr static char_type CARET     = '^';
    constexpr static char_type DASH      = '-';
    constexpr static char_type RBRACKET  = ']';
    constexpr static char_type OR        = '|';
    constexpr static char_type STAR      = '*';
    constexpr static char_type BACKSLASH = '\\';

    constexpr static char_type LBRACE    = '{';
    constexpr static char_type COMMA     = ',';
    constexpr static char_type RBRACE    = '}';
    constexpr static char_type QUESTION  = '?';
    constexpr static char_type PLUS      = '+';
    constexpr static char_type DOT       = '.';

    constexpr static char_type MIN = std::numeric_limits<char_type>::min();
    constexpr static char_type MAX = std::numeric_limits<char_type>::max();
};

template<typename CharT>
std::string char_to_string(CharT c) {
    return std::to_string(c);;
}
template<>
std::string char_to_string<char>(char c) {
    return std::string(1, c);
}

template<typename CharT>
class Regex2BasicConvertor {
private:
    using traits = character_traits<CharT>;
    using char_type = typename traits::char_type;
    std::vector<size_t> m_lparen_pos;
    std::vector<char_type> __result;
    bool escaping;
    bool endding;
    size_t m_last_lparen_pos;
    static constexpr size_t npos = std::numeric_limits<size_t>::max();

    bool m_in_bracket_mode;
    bool m_escaping_in_bracket_mode;

    bool m_in_brace_mode;
    bool m_brace_got_comma;
    int  m_brace_count_low;
    int  m_brace_count_up;
    static constexpr size_t brace_infinity = std::numeric_limits<decltype(m_brace_count_up)>::max();
    std::vector<char_type> __copy_content;

    std::vector<char_type> get_last_node(size_t last_lparen_pos)
    {
        if (this->__result.empty())
            throw std::runtime_error("unexpected {} / + / ?");

        if (last_lparen_pos == npos) {
            const auto __rsize = __result.size();
            if (__rsize >= 2 && 
                this->__result[__rsize - 1] == traits::BACKSLASH &&
                this->__result[__rsize - 2] == traits::BACKSLASH)
            {
                return std::vector<char_type>({  traits::BACKSLASH, traits::BACKSLASH });
            } else {
                return std::vector<char_type>({  this->__result.back() });
            }
        } else {
            assert(this->__result.back() == traits::LPAREN);
            std::vector<char_type> ans;
            std::copy(this->__result.begin() + last_lparen_pos, this->__result.end(), std::back_inserter(ans));
            return ans;
        }
    }

    bool handle_bracket_mode(char_type c) {
        if (!this->m_in_bracket_mode)
            return false;

        this->__result.push_back(c);

        if (c == traits::BACKSLASH) {
            this->m_escaping_in_bracket_mode = !this->m_escaping_in_bracket_mode;
        } else {
            this->m_escaping_in_bracket_mode = false;
            this->m_in_bracket_mode = c != traits::RBRACKET;
        }

        return true;
    }

    void enter_brace_mode() {
        this->m_brace_count_low = 0;
        this->m_brace_count_up = 0;
        this->m_in_brace_mode = true;
        this->m_brace_got_comma = false;
    }

    bool handle_brace_mode(char_type c) {
        if (!this->m_in_brace_mode)
            return false;

        if (c == traits::COMMA) {
            if (this->m_brace_got_comma)
                throw std::runtime_error("unexpected , in brace");
            this->m_brace_got_comma = true;
            this->m_brace_count_up=brace_infinity;
            return true;
        } else if (c == traits::RBRACE) {
            this->__result.resize(this->__result.size() - this->__copy_content.size());
            for (size_t i=0; i<this->m_brace_count_low; i++) {
                this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
            }

            if (this->m_brace_got_comma) {
                if (this->m_brace_count_up < this->m_brace_count_low)
                    throw std::runtime_error("range error");

                if (this->m_brace_count_up == brace_infinity) {
                    this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
                    this->__result.push_back(traits::STAR);
                } else {
                    const auto opt_count = this->m_brace_count_up - this->m_brace_count_low;
                    for (size_t i=0; i<opt_count; i++) {
                        this->__result.push_back(traits::LPAREN);
                        this->__result.push_back(traits::OR);
                        this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
                    }
                    std::vector<char_type> nrparen(opt_count, traits::RPAREN);
                    this->__result.insert(this->__result.end(), nrparen.begin(), nrparen.end());
                }
            }

            this->m_in_brace_mode = false;
            return true;
        }

        if (c < '0' || c > '9')
            throw std::runtime_error("brace mode: expect number, bug get '" + char_to_string(c) + "'");

        if (this->m_brace_got_comma) {
            if (this->m_brace_count_up == brace_infinity)
                this->m_brace_count_up = 0;

            this->m_brace_count_up *= 10;
            this->m_brace_count_up += c - '0';
        } else {
            this->m_brace_count_low *= 10;
            this->m_brace_count_low += c - '0';
        }

        return true;
    }

public:
    Regex2BasicConvertor():
        escaping(false), endding(false),
        m_last_lparen_pos(npos),
        m_in_bracket_mode(false), m_in_brace_mode(false) {}

    void feed(char_type c) {
        assert(!this->endding);
        auto last_lparen_pos = this->m_last_lparen_pos;
        this->m_last_lparen_pos = npos;

        if (this->handle_bracket_mode(c))
            return;

        if (this->handle_brace_mode(c))
            return;

        if (this->escaping) {
            switch (c) {
                case traits::LBRACE:
                case traits::RBRACE:
                case traits::QUESTION:
                case traits::PLUS:
                case traits::DOT:
                    this->__result.push_back(c);
                    break;
                default:
                    this->__result.push_back(traits::BACKSLASH);
                    this->__result.push_back(c);
                    break;
            }

            this->escaping = false;
            return;
        }

        switch (c) {
            case traits::LPAREN:
                this->m_lparen_pos.push_back(this->__result.size());
                this->__result.push_back(c);
                break;
            case traits::RPAREN:
                if (this->m_lparen_pos.empty())
                    throw std::runtime_error("unmatched right parenthese");
                this->__result.push_back(c);
                this->m_last_lparen_pos = this->m_lparen_pos.back();
                this->m_lparen_pos.pop_back();
                break;
            case traits::BACKSLASH:
                this->escaping = true;
                break;
            case traits::LBRACE:
                this->enter_brace_mode();
                this->__copy_content = this->get_last_node(last_lparen_pos);
                break;
            case traits::RBRACE:
                throw std::runtime_error("unexcepted rbrace");
                break;
            case traits::LBRACKET:
                this->m_in_bracket_mode = true;
                this->m_escaping_in_bracket_mode = false;
                this->__result.push_back(c);
                break;
            case traits::QUESTION: {
                auto last_node = this->get_last_node(last_lparen_pos);
                assert(this->__result.size() >= last_node.size());
                this->__result.resize(this->__result.size() - last_node.size());
                this->__result.push_back(traits::LPAREN);
                this->__result.push_back(traits::OR);
                this->__result.insert(this->__result.end(), last_node.begin(), last_node.end());
                this->__result.push_back(traits::RPAREN);
            } break;
            case traits::PLUS: {
                auto last_node = this->get_last_node(last_lparen_pos);
                this->__result.insert(this->__result.end(), last_node.begin(), last_node.end());
                this->__result.push_back(traits::STAR);
            } break;
            case traits::DOT:
                this->__result.push_back(traits::LBRACKET);
                this->__result.push_back(traits::MIN);
                this->__result.push_back(traits::DASH);
                this->__result.push_back(traits::MAX);
                this->__result.push_back(traits::RBRACKET);
                break;
            default:
                this->__result.push_back(c);
                break;
        }
    }

    std::vector<char_type>& end() {
        assert(!this->endding);
        this->endding = true;

        if (this->escaping)
            throw std::runtime_error("Regex: unclosed escape");

        return __result;
    }

    static std::vector<char_type> convert(const std::vector<char_type>& regex) {
        Regex2BasicConvertor conv;
        for (auto c : regex)
            conv.feed(c);
        return conv.end();
    }
};

enum ExprNodeType {
    ExprNodeType_Empty, ExprNodeType_Group, ExprNodeType_CharRange,
    ExprNodeType_Concatenation, ExprNodeType_Union,  ExprNodeType_KleeneStar
};

template<typename CharT>
class ExprNode {
public:
    virtual ExprNodeType node_type() const = 0;
    virtual std::string to_string() const = 0;
    virtual ~ExprNode() = default;
};

template<typename CharT>
class ExprNodeEmpty : public ExprNode<CharT> {
public:
    virtual ExprNodeType node_type() const override { return ExprNodeType_Empty; }
    virtual std::string to_string() const override {
        return "";
    }
    virtual ~ExprNodeEmpty() override = default;
};

template<typename CharT>
class ExprNodeGroup : public ExprNode<CharT> {
private:
    std::shared_ptr<ExprNode<CharT>> _next;

public:
    ExprNodeGroup(std::shared_ptr<ExprNode<CharT>> next) : _next(next) {}

    const std::shared_ptr<ExprNode<CharT>> next() const { return _next; }
    std::shared_ptr<ExprNode<CharT>> next() { return _next; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Group; }
    virtual std::string to_string() const override {
        return "(" + this->_next->to_string() + ")";
    }
    virtual ~ExprNodeGroup() override = default;
};

template<typename CharT>
class ExprNodeCharRange : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = typename traits::char_type;
    char_type _min, _max;

public:
    ExprNodeCharRange(char_type min, char_type max) : _min(min), _max(max)
    {
        assert(min <= max);
        assert(traits::MIN <= min && min <= traits::MAX);
        assert(traits::MIN <= max && max <= traits::MAX);
    }

    char_type min() const { return _min; }
    char_type max() const { return _max; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_CharRange; }
    virtual std::string to_string() const override {
        if (this->_min == this->_max)
            return char_to_string(this->_min);
        else
            return char_to_string(this->_min) + "-" + char_to_string(this->_max);
    }
    virtual ~ExprNodeCharRange() override = default;
};

template<typename CharT>
class ExprNodeConcatenation : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = typename traits::char_type;
    std::vector<std::shared_ptr<ExprNode<CharT>>> children;

public:
    ExprNodeConcatenation() {}

    const std::vector<std::shared_ptr<ExprNode<CharT>>>& children_ref() const { return children; }
    const size_t size() const { return children.size(); }

    void add_child(std::shared_ptr<ExprNode<CharT>> child) { children.push_back(child); }
    auto& back() { return children.back(); }
    const auto& back() const { return children.back(); }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Concatenation; }
    virtual std::string to_string() const override {
        std::string result;
        for (auto& child : children)
            result += child->to_string();
        return result;
    }
    virtual ~ExprNodeConcatenation() override = default;
};

template<typename CharT>
class ExprNodeUnion : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = typename traits::char_type;
    std::vector<std::shared_ptr<ExprNode<CharT>>> children;

public:
    ExprNodeUnion() {}

    const std::vector<std::shared_ptr<ExprNode<CharT>>>& children_ref() const { return children; }
    const size_t size() const { return children.size(); }

    void add_child(std::shared_ptr<ExprNode<CharT>> child) { children.push_back(child); }
    auto& back() { return children.back(); }
    const auto& back() const { return children.back(); }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Union; }
    virtual std::string to_string() const override {
        std::string result;
        for (auto& child : children)
            result += child->to_string() + "|";
        result.erase(result.end() - 1);
        return result;
    }
    virtual ~ExprNodeUnion() override = default;
};

template<typename CharT>
class ExprNodeKleeneStar : public ExprNode<CharT> {
private:
    std::shared_ptr<ExprNode<CharT>> _next;

public:
    ExprNodeKleeneStar(std::shared_ptr<ExprNode<CharT>> next) : _next(next) {}

    const std::shared_ptr<ExprNode<CharT>> next() const { return _next; }
    std::shared_ptr<ExprNode<CharT>> next() { return _next; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_KleeneStar; }
    virtual std::string to_string() const override {
        return this->_next->to_string() + "*";
    }
    virtual ~ExprNodeKleeneStar() override = default;
};

template<typename CharT>
class RegexNodeTreeGenerator {
private:
    using traits = character_traits<CharT>;
    using char_type = typename traits::char_type;
    using node_type = std::shared_ptr<ExprNode<char_type>>;;
    struct StackValueState {
        std::shared_ptr<ExprNode<char_type>> node;

        StackValueState(): node(std::make_shared<ExprNodeEmpty<char_type>>()) {}
    };
    std::vector<StackValueState> _stack;
    bool _endding;
    bool _escaping;

    node_type push_node_to_node(node_type oldnode, node_type node)
    {
        assert(oldnode);
        node_type ret;

        switch (oldnode->node_type())
        {
            case ExprNodeType_Concatenation: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeConcatenation<char_type>>(oldnode);
                ptr->add_child(node);
                ret = oldnode;
            } break;
            case ExprNodeType_Union: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(oldnode);
                auto& last = ptr->back();
                last = push_node_to_node(last, node);
                ret = oldnode;
            } break;
            case ExprNodeType_Group:
            case ExprNodeType_CharRange:
            case ExprNodeType_KleeneStar: {
                auto newnode = std::make_shared<ExprNodeConcatenation<char_type>>();
                newnode->add_child(oldnode);
                newnode->add_child(node);
                ret = newnode;
            } break;
            case ExprNodeType_Empty: {
                ret = node;
            } break;
            default:
                assert("regex: invalid node type");
        }

        return ret;
    }
    void push_node(node_type node) {
        auto& stacktop = _stack.back();
        stacktop.node = push_node_to_node(stacktop.node, node);
    }
    void push_char(char_type c) {
        auto node = std::make_shared<ExprNodeCharRange<char_type>>(c, c);
        push_node(node);
    }
    void to_union_node() {
        auto& stacktop = _stack.back();
        assert (stacktop.node != nullptr);

        switch (stacktop.node->node_type())
        {
            case ExprNodeType_Empty:
            case ExprNodeType_Group:
            case ExprNodeType_CharRange:
            case ExprNodeType_KleeneStar:
            case ExprNodeType_Concatenation: {
                auto newnode = std::make_shared<ExprNodeUnion<char_type>>();
                newnode->add_child(stacktop.node);
                stacktop.node = newnode;
            } break;
            case ExprNodeType_Union: break;
            default:
                assert("regex: invalid node type");
        }
    }
    node_type node_to_kleen_start_node(node_type node) {
        assert(node != nullptr);
        node_type ret;

        switch (node->node_type())
        {
            case ExprNodeType_Group:
            case ExprNodeType_KleeneStar:
            case ExprNodeType_CharRange: {
                auto newnode = std::make_shared<ExprNodeKleeneStar<char_type>>(node);
                ret = newnode;
            } break;
            case ExprNodeType_Concatenation: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeConcatenation<char_type>>(node);
                assert(ptr->size() >= 1);
                auto& back = ptr->back();
                back = node_to_kleen_start_node(back);
                ret = node;
            } break;
            case ExprNodeType_Union: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(node);
                assert(ptr->size() >= 1);
                auto& back = ptr->back();
                back = node_to_kleen_start_node(back);
                ret = node;
            } break;
            case ExprNodeType_Empty:
                throw std::runtime_error("regex: invalid kleene star node");
            default:
                assert("regex: invalid node type");
        }

        return ret;
    }
    void to_kleene_star_node() {
        auto& stacktop = _stack.back();
        assert(stacktop.node != nullptr);
        stacktop.node = node_to_kleen_start_node(stacktop.node);
        assert(stacktop.node != nullptr);
    }

    bool m_in_bracket_mode;
    bool m_escaping_in_bracket_mode;
    bool m_bracket_reversed;
    size_t m_bracket_eat_count;
    enum BracketState {
        BracketState_None, BracketState_One,
        BracketState_Dashed, BracketState_Two,
    } m_bracket_state;
    std::vector<std::pair<char_type, char_type>> m_bracket_ranges;
    char_type m_range_low, m_range_up;

    void enter_bracket_mode() {
        m_in_bracket_mode = true;
        m_escaping_in_bracket_mode = false;
        m_bracket_eat_count = 0;
        m_bracket_reversed = false;
        m_bracket_state = BracketState_None;
        m_bracket_ranges.clear();
    }
    void leave_bracket_mode() {
        this->push_bracket_range();
        m_in_bracket_mode = false;

        std::sort(m_bracket_ranges.begin(), m_bracket_ranges.end());
        auto merged_ranges = merge_sorted_range(m_bracket_ranges);
        if (merged_ranges.empty())
            throw std::runtime_error("Regex: empty bracket");

        if (merged_ranges.size() == 1) {
            auto node = std::make_shared<ExprNodeCharRange<char_type>>(merged_ranges[0].first, merged_ranges[0].second);
            this->push_node(std::make_shared<ExprNodeGroup<char_type>>(node));
        } else {
            auto node = std::make_shared<ExprNodeUnion<char_type>>();
            for (auto& range : merged_ranges) {
                auto node_range = std::make_shared<ExprNodeCharRange<char_type>>(range.first, range.second);
                node->add_child(node_range);
            }
            this->push_node(std::make_shared<ExprNodeGroup<char_type>>(node));
        }
    }
    void push_bracket_range() {
        switch (this->m_bracket_state) {
            case BracketState_None:
                break;
            case BracketState_One: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, this->m_range_low));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
                   if (this->m_range_low < traits::MAX)
                        this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low+1, traits::MAX));
               }
            } break;
            case BracketState_Dashed: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, traits::MAX));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
               }
            } break;
            case BracketState_Two: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, this->m_range_up));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
                   if (this->m_range_up < traits::MAX)
                        this->m_bracket_ranges.push_back(std::make_pair(this->m_range_up+1, traits::MAX));
               }
               this->m_bracket_state = BracketState_None;
            } break;
            default:
               throw std::runtime_error("invalid bracket state");
        }
    }
    bool handle_bracket_mode(char_type c) {
        if (!this->m_in_bracket_mode)
            return false;

        if (this->m_bracket_eat_count++ == 0 && c == traits::CARET) {
            this->m_bracket_reversed = true;
            return true;
        }

        const bool bracket_end = !this->m_escaping_in_bracket_mode && c == traits::RBRACKET;
        if (bracket_end) {
            this->leave_bracket_mode();
            return true;
        }

        if (this->m_escaping_in_bracket_mode) {
            if (c != traits::RBRACKET && c != traits::BACKSLASH)
                throw std::runtime_error("unexpected escape seqeuence");
            this->m_escaping_in_bracket_mode = false;
        } else if (c == traits::BACKSLASH) {
            this->m_escaping_in_bracket_mode = true;
            return true;
        }

        switch (this->m_bracket_state) {
            case BracketState_None:
            {
                this->m_range_low = c;
                this->m_bracket_state = BracketState_One;
            } break;
            case BracketState_One:
            {
                if (c == traits::DASH) {
                    this->m_bracket_state = BracketState_Dashed;
                } else {
                    this->push_bracket_range();
                    this->m_range_low = c;
                }
            } break;
            case BracketState_Dashed:
            {
                this->m_range_up = c;
                this->m_bracket_state = BracketState_Two;
                this->push_bracket_range();
                this->m_bracket_state = BracketState_None;
            } break;
            default:
                throw std::runtime_error("unexpected bracket state");
        }

        return true;
    }

public:
    RegexNodeTreeGenerator():
       _endding(false), _escaping(false),
        m_in_bracket_mode(false), m_escaping_in_bracket_mode(false),
        _stack()
    {
        this->_stack.push_back(StackValueState());
    }

    void feed(char_type c) {
        assert(!this->_endding);

        if (this->handle_bracket_mode(c))
            return;

        if (this->_escaping) {
            if (c != traits::LPAREN && c != traits::RPAREN &&
                c != traits::LBRACKET && c != traits::CARET &&
                c != traits::DASH && c != traits::RBRACKET &&
                c != traits::OR && c != traits::STAR &&
                c != traits::BACKSLASH)
            {
                throw std::runtime_error("unexpected escape seqeuence");
            }

            this->_escaping = false;
            this->push_char(c);
            return;
        }

        switch (c) {
            case traits::LPAREN:
            {
                this->_stack.push_back(StackValueState());
            } break;
            case traits::RPAREN:
            {
                if (this->_stack.size() < 2)
                    throw std::runtime_error("unexpected ')'");

                auto top = this->_stack.back();
                this->_stack.pop_back();
                auto group = std::make_shared<ExprNodeGroup<char_type>>(top.node);
                this->push_node(group);
            } break;
            case traits::LBRACKET:
            {
                this->enter_bracket_mode();
            } break;
            case traits::RBRACKET:
            {
                throw std::runtime_error("unexpected ]");
            } break;
            case traits::OR:
            {
                this->to_union_node();
                auto& top = this->_stack.back();
                auto node = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(top.node);
                assert(node != nullptr);
                node->add_child(std::make_shared<ExprNodeEmpty<char_type>>());
            } break;
            case traits::STAR:
            {
                this->to_kleene_star_node();
            } break;
            case traits::BACKSLASH:
            {
                this->_escaping = true;
            } break;
            default:
                this->push_char(c);
        }
    }

    node_type end() {
        if (this->_endding)
            throw std::runtime_error("already endding");

        this->_endding = true;

        if (this->_stack.size() != 1)
            throw std::runtime_error("unexpected end");

        auto top = this->_stack.back();
        this->_stack.pop_back();
        return top.node;
    }

    static node_type parse(const std::vector<char_type>& str) {
        RegexNodeTreeGenerator<char_type> gen;
        for (auto c: str) {
            gen.feed(c);
        }
        return gen.end();
    }
};

#endif // _DC_PARSER_REGEX_HPP_
