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

TEST(NFA, basic_test) {
    vector<tuple<string,vector<string>,vector<string>>> test_cases = {
        {"aa*", { "aa", "a", "aaa" }, { "", "b", "aab" }},
        {"a*", { "", "a", "aa" }, { "aabaa", "b" }},
        {"ab", { "ab" }, { "ba", "b", "a", "" }},
        {"aa", { "aa" }, { "ab", "bb", "aaa", "a", "" }},
        {"a", { "a" }, { "aa", "" }},
        {"a|b|c|d|e", { "a", "b", "c", "d", "e" }, { "", "ab", "ba", "de", "ed", "ee", "dd" }},
        {"(a)", { "a" }, { "", "aa" }},
        {"(a|bd)", { "bd", "a" }, { "b", "d", "ab", "ad" }},
        {"(a())", { "a" }, { "", "aa" }},
        {"([a-bc])", { "a", "b", "c" }, { "", "aa", "bb", "cc", "ab" }},

        {"a?", { "a", "" }, { "aa" }},
        {"a+", { "aaa", "a", "aa", "aaaaa" }, { "", "aabaa" }},
        {"a{,}", { "a", "" }, { "ab" }},
        {"a{2,4}", { "aa", "aaa", "aaaa" }, { "a", "aaaaa", "" }},

        {"(a(a(a(a(a)))))", { "aaaaa" }, { "a" } },
    };

    for (auto& testcase : test_cases) {
        auto& re = get<0>(testcase);
        auto& accepts = get<1>(testcase);
        auto& rejects = get<2>(testcase);

        auto matcher = NFAMatcher<char>(vector<char>(re.begin(), re.end()));

        for (auto& accept: accepts) {
            matcher.reset();
            ASSERT_TRUE(matcher.test(accept.begin(), accept.end()))
                << re << ": " << accept << endl
                << matcher.to_string() << endl;
        }

        for (auto& reject: rejects) {
            matcher.reset();
            ASSERT_FALSE(matcher.test(reject.begin(), reject.end()))
                << re << ": " << reject << endl
                << matcher.to_string() << endl;
        }
    }
}
