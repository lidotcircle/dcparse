#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "regex/regex.hpp"
using namespace std;


TEST(regex, regex_basic_node_generator) {
    vector<pair<string,string>> test_cases = {
        {"a", "a"},
        {"a*", "a*"},
        {"a|b|c", "a|b|c"},
        {"a|b", "a|b"},
        {"a|b|c|", "a|b|c|"},
        {"a|b|c|d", "a|b|c|d"},
        {"a|b|c|d|", "a|b|c|d|"},
        {"a|b|c|d|e", "a|b|c|d|e"},

        {"\\(\\)\\[\\^\\-\\]\\|\\*\\\\", "()[^-]|*\\"},
        {"\\(\\)\\[\\]\\|\\*\\\\^-", "()[]|*\\^-"},

        {"(a)", "(a)"},
        {"(a|bd)", "(a|bd)"},
        {"(a|bd)|c|d|(a-b)", "(a|bd)|c|d|(a-b)"},
        {"(a())", "(a())"},
        {"(a(b))", "(a(b))"},

        {"([a-bc])", "((a-c))"},
    };

    for (auto& test_case : test_cases) {
        auto& regex = test_case.first;
        auto node = RegexNodeTreeGenerator<char>::parse(vector<char>(regex.begin(), regex.end()));
        ASSERT_EQ(node->to_string(), test_case.second);
    }
}

TEST(regex, regex_extend_node_generator) {
    vector<pair<string,string>> test_cases = {
        {"a", "a"},
        {"a*", "a*"},
        {"aa*", "aa*"},
        {"a|b|c", "a|b|c"},
        {"a|b", "a|b"},
        {"a|b|c|", "a|b|c|"},
        {"a|b|c|d", "a|b|c|d"},
        {"a|b|c|d|", "a|b|c|d|"},
        {"a|b|c|d|e", "a|b|c|d|e"},

        {"\\(\\)\\[\\^\\-\\]\\|\\*\\\\", "()[^-]|*\\"},
        {"\\(\\)\\[\\]\\|\\*\\\\^-", "()[]|*\\^-"},

        {"(a)", "(a)"},
        {"(a|bd)", "(a|bd)"},
        {"(a|bd)|c|d|(a-b)", "(a|bd)|c|d|(a-b)"},
        {"(a())", "(a())"},
        {"(a(b))", "(a(b))"},

        {"([a-bc])", "((a-c))"},
        {"([a-bd])", "((a-b|d))"},

        {"a?", "(|a)"},
        {"a+", "aa*"},
        {"a{2}", "aa"},
        {"a{,}", "a*"},
        {"a{2,4}", "aa(|a(|a))"},

        {"(ab)*", "(ab)*"},
        {"(ab){2}", "(ab)(ab)"},
        {"(ab){2,}", "(ab)(ab)(ab)*"},

        {"(ab(ab(ab)))*", "(ab(ab(ab)))*"},
        {"(ab(ab(ab))){1}", "(ab(ab(ab)))"},
        {"(ab(ab(ab))){1,2}", "(ab(ab(ab)))(|(ab(ab(ab))))"},
        {"(ab(ab(ab))){1,}", "(ab(ab(ab)))(ab(ab(ab)))*"},
    };

    for (auto& test_case : test_cases) {
        auto regex = Regex2BasicConvertor<char>::convert(vector<char>(test_case.first.begin(),test_case.first.end()));
        auto node = RegexNodeTreeGenerator<char>::parse(regex);
        ASSERT_EQ(node->to_string(), test_case.second);
    }
}
