#include <gtest/gtest.h>
#include "lexer/token.h"
using namespace std;

class T1 : public LexerToken {
public:
    T1(): LexerToken() {}
    ~T1() = default;
};
class T2 : public LexerToken {
public:
    T2(): LexerToken() {}
    ~T2() = default;
};

TEST(token, tokenid_basic) {
    T1 t1;
    T2 t2;

    EXPECT_NE(t1.charid(), t2.charid());
    EXPECT_EQ(CharID<T1>(), t1.charid());
    EXPECT_EQ(CharID<T2>(), t2.charid());
}

