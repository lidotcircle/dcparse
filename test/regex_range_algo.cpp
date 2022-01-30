#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include <random>
#include "regex/misc.hpp"
using namespace std;


TEST(algo, regex_range) {
    std::vector<std::pair<size_t,size_t>> test_set;
    std::default_random_engine generator;
    std::uniform_int_distribution<size_t> distribution(0,100000);

    for (size_t i=0;i<10000;i++) {
        auto a = distribution(generator);
        auto b = distribution(generator);
        if (a > b)
            std::swap(a,b);

        test_set.push_back(std::make_pair(a,b));
    }

    std::sort(test_set.begin(),test_set.end());
    auto m1 = merge_sorted_range(test_set);
    auto m2 = split_ranges_to_units(test_set);
    auto m3 = merge_sorted_range(m2);

    EXPECT_EQ(m1,m3);
    auto f1 = m2[0];
    for (size_t i=1;i<m2.size();i++) {
        ASSERT_LT(f1.second,m2[i].first);
        f1 = m2[i];
    }

    for (auto& rg: test_set) {
        auto lb = std::lower_bound(m2.begin(),m2.end(),rg.first,
                                   [](const auto& r, auto v) { return r.first < v;});
        ASSERT_EQ(lb->first,rg.first);
        ASSERT_LE(lb->second,rg.second);
    }
}
