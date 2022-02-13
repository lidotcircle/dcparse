#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "regex/regex.hpp"
using namespace std;


TEST(DFA, basic_dfa_test) {
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

        {"(!1234)", { "431", "" }, { "1234" }},
        {"a(!d)f",  { "acf" }, { "adf" }},
        {"/\\*(!\\*/)\\*/", { "/* asdf */" }, { "", "/* asdf */ " } },

        {"(a(a(a(a(a)))))", { "aaaaa" }, { "a" } },
        {"[^0-9]+", { "abc" }, { "a1234" } },
    };

    for (auto& testcase : test_cases) {
        auto& re = get<0>(testcase);
        auto& accepts = get<1>(testcase);
        auto& rejects = get<2>(testcase);

        auto matcher = DFAMatcher<char>(vector<char>(re.begin(), re.end()));

        for (auto& accept: accepts) {
            matcher.reset();
            ASSERT_TRUE(matcher.test(accept.begin(), accept.end()))
                << re << ": " << accept << endl
                << matcher.to_string();
        }

        for (auto& reject: rejects) {
            matcher.reset();
            ASSERT_FALSE(matcher.test(reject.begin(), reject.end()))
                << re << ": " << reject << endl
                << matcher.to_string();
        }
    }
}
