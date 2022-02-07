#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "regex/regex_utf8.h"
using namespace std;


TEST(regex_utf8, basic) {
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
        {".*", { "意见反馈" }, { } },
        {"意见反馈", { "意见反馈" }, { "意见反", "意见反", "a意见反馈", "意见反馈b" } },
    };

    for (auto& testcase : test_cases) {
        auto& re = get<0>(testcase);
        auto& accepts = get<1>(testcase);
        auto& rejects = get<2>(testcase);

        auto matcher = RegExpUTF8(re);
        auto m2 = RegExpUTF8::compiled(re);

        for (auto& accept: accepts) {
            ASSERT_TRUE(matcher.test(accept.begin(), accept.end()))
                << "ACCEPT >> " <<  re << ": " << accept << endl
                << matcher.to_string() << endl;

            ASSERT_TRUE(m2.test(accept.begin(), accept.end()))
                << "DFA ACCEPT >> " <<  re << ": " << accept << endl
                << m2.to_string() << endl;
        }

        for (auto& reject: rejects) {
            ASSERT_FALSE(matcher.test(reject.begin(), reject.end()))
                << "REJECT >> " << re << ": " << reject << endl;

            ASSERT_FALSE(m2.test(reject.begin(), reject.end()))
                << "DFA REJECT >> " << re << ": " << reject << endl;
        }
    }
}