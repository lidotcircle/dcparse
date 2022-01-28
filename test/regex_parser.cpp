#include <gtest/gtest.h>
#include <vector>
#include "regex/regex.hpp"
using namespace std;

TEST(regex, regex_basic) {
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

        {"([a-bc])", "((a-b|c))"},
    };

    for (auto& test_case : test_cases) {
        auto& regex = test_case.first;
        auto node = RegexNodeTreeGenerator<char>::parse(vector<char>(regex.begin(), regex.end()));
        ASSERT_EQ(node->to_string(), test_case.second);
    }
}

