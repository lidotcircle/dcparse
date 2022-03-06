#include <gtest/gtest.h>
#include "c_parser.h"
#include <iostream>
using namespace std;


TEST(RuleTransitionTable, CParserBasic) {
    EXPECT_NO_THROW(
        cparser::CParser parser;
    );
}
