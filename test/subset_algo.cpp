#include <gtest/gtest.h>
#include "algo.hpp"
using namespace std;


TEST(subset, subsetalgo) {
    SubsetOf<int> s({1, 2, 3});

    set<set<int>> subsetOfs = {
        {1, 2, 3},
        {1, 2},
        {1, 3},
        {1},
        {2, 3},
        {2},
        {3},
        {},
    };

    for (auto s1 = s(); s1.has_value() ; s1=s()) {
        EXPECT_NE(subsetOfs.find(s1.value()), subsetOfs.end());
        subsetOfs.erase(s1.value());
    }
    EXPECT_EQ(subsetOfs.size(), 0);
}
