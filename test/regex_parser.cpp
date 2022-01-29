#include <gtest/gtest.h>
#include <vector>
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

        {"([a-bc])", "((a-b|c))"},
    };

    for (auto& test_case : test_cases) {
        auto& regex = test_case.first;
        auto node = RegexNodeTreeGenerator<char>::parse(vector<char>(regex.begin(), regex.end()));
        ASSERT_EQ(node->to_string(), test_case.second);
    }
}

TEST(regex, regex_convert) {
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

        {"(a)", "(a)"},
        {"(a|bd)", "(a|bd)"},
        {"(a|bd)|c|d|(a-b)", "(a|bd)|c|d|(a-b)"},
        {"(a())", "(a())"},
        {"(a(b))", "(a(b))"},

        {"([a-bc])", "([a-bc])"},

        {"a?", "(|a)"},
        {"a+", "aa*"},
        {"a{1}", "a"},
        {"a{2}", "aa"},
        {"a{2,}", "aaa*"},
        {"a{,}", "a*"},
        {"a{2,4}", "aa(|a(|a))"},

        {"(ab)*", "(ab)*"},
        {"(ab){2}", "(ab)(ab)"},
        {"(ab){2,}", "(ab)(ab)(ab)*"},

        {"(ab(ab(ab)))*", "(ab(ab(ab)))*"},
        {"(ab(ab(ab))){1}", "(ab(ab(ab)))"},
        {"(ab(ab(ab))){1,2}", "(ab(ab(ab)))(|(ab(ab(ab))))"},
        {"(ab(ab(ab))){1,}", "(ab(ab(ab)))(ab(ab(ab)))*"},

        {"\\{\\}\\?\\+\\.", "{}?+."},
    };

    for (auto& test_case : test_cases) {
        auto regex = Regex2BasicConvertor<char>::convert(vector<char>(test_case.first.begin(),test_case.first.end()));
        ASSERT_EQ(string(regex.begin(), regex.end()), test_case.second);
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

        {"([a-bc])", "((a-b|c))"},

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

TEST(NFA, basic_test) {
    vector<pair<string,string>> accepts = {
        {"a", "a"},
        {"a|b|c|d|e", "e"},
        {"(a)", "a"},
        {"(a|bd)", "bd"},
        {"(a())", "a"},
        {"([a-bc])", "a"},

        {"a?", "a"},
        {"a+", "aaa"},
        {"a{2}", "aa"},
        {"a{,}", "a"},
        {"a{2,4}", "aa"},
    };

    for (auto& accept : accepts) {
        auto matcher = NFAMatcher<char>(vector<char>(accept.first.begin(), accept.first.end()));

        auto& acceptval = accept.second;
        ASSERT_TRUE(matcher.test(acceptval.begin(), acceptval.end()));
    }
}
