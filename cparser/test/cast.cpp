#include <gtest/gtest.h>
#include "c_lexer_parser.h"
#include "c_reporter.h"
#include <iostream>
#include <string>
#include <vector>
using namespace cparser;
using namespace std;


TEST(should_accpet, CAST) {
    CLexerParser parser;

    vector<string> test_cases = {
        "int a;",
        "_Static_assert(1, \"\");",
    };

    for (auto t: test_cases) {
        auto reporter = make_shared<SemanticReporter>();
        ASSERT_NO_THROW(
            for (auto c: t) {
                parser.feed(c);
            }

            auto tunit = parser.end();
            ASSERT_NE(tunit, nullptr);
            tunit->check_constraints(reporter);
        ) << t;

        for (auto err: *reporter)
            cerr << err->what() << endl;
        ASSERT_EQ(reporter->size(), 0) << t;
        parser.reset();
    }
}

TEST(should_reject, CAST) {
    CLexerParser parser;

#define EXPECT_REJECT_LIST \
    EENTRY("_Static_assert(0, \"hello, this is a static_assert failure\");", StaticAssertFailed) \
    EENTRY("int a; double a;", Redefinition) \

    shared_ptr<SemanticError> err;
    const auto eval_code = [&](string code) {
        err = nullptr;
        auto reporter = make_shared<SemanticReporter>();
        ASSERT_NO_THROW(
            for (auto c: code) {
                parser.feed(c);
            }

            auto tunit = parser.end();
            ASSERT_NE(tunit, nullptr);
            try {
                tunit->check_constraints(reporter);
            } catch (CErrorStaticAssert&) {}
        ) << code;

        EXPECT_GT(reporter->size(), 0) << code;
        if (reporter->size() > 0)
            err = (*reporter)[0];

        parser.reset();
    };


#define EENTRY(str, type) \
    { \
        eval_code(str); \
        ASSERT_NE(err, nullptr) << str; \
        auto serr = dynamic_pointer_cast<SemanticError##type>(err); \
        ASSERT_NE(serr, nullptr) << str << endl << "    " << err->what(); \
    }
EXPECT_REJECT_LIST
#undef EENTRY
}
