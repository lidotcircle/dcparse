#include <gtest/gtest.h>
#include "c_parser.h"
#include <iostream>
using namespace std;


TEST(RuleTransitionTable, Basic) {
    cparser::CParser parser;
    parser.setDebugStream(cout);
}
