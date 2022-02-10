#ifndef _DC_PARSER_REGEX_EXPR_HPP_
#define _DC_PARSER_REGEX_EXPR_HPP_

#include <vector>
#include <stdexcept>
#include <assert.h>
#include "./regex_char.hpp"


template<typename CharT>
class RegexPatternEscaper {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using regex_char = RegexPatternChar<char_type>;

public:
    RegexPatternEscaper() = delete;

    template<typename Iterator>
    static std::vector<char_type> convert(Iterator first, Iterator last)
    {
        std::vector<regex_char> result;
        bool escaped = false;

        for (; first != last; ++first) {
            if (escaped) {
                escaped = false;
                result.push_back(regex_char::escape(*first));
            } else {
                if (*first == traits::BACKSLASH) {
                    escaped = true;
                } else {
                    result.push_back(regex_char::unescape(*first));
                }
            }
        }

        if (escaped)
            throw std::invalid_argument("unterminated escape sequence");

        return result;
    }
};

template<typename CharT>
class Regex2BasicConvertor {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
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
            assert(this->__result.back() == traits::RPAREN);
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
                throw std::runtime_error("unexpected rbrace");
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

#endif // _DC_PARSER_REGEX_EXPR_H_
