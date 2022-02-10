#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "regex/regex.hpp"
using namespace std;

TEST(regex, escaping_test) {
    vector<pair<string,string>> test_cases = {
        { "abc", "abc" },
        { "\\abc", "\\abc" },
        { "a\\bc", "a\\bc" },
        { "\\\\abc", "\\\\abc" },
    };

    for (auto& test_case : test_cases) {
        auto ff = RegexPatternEscaper<char>::convert(test_case.first.begin(), test_case.first.end());
        auto rs = pattern_to_string(ff);
        EXPECT_EQ(rs, test_case.second);
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
