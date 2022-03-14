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
        "_Static_assert(sizeof(char) == 1, \"\");",
        "_Static_assert(sizeof(int) == 4, \"\");",
        "_Static_assert(sizeof(long) == 8, \"\");",
        "_Static_assert(sizeof(long long) == 8, \"\");",
        "_Static_assert(sizeof(short) == 2, \"\");",
        "_Static_assert(sizeof(signed char) == 1, \"\");",
        "_Static_assert(sizeof(unsigned char) == 1, \"\");",
        "_Static_assert(sizeof(void*) == 4 || sizeof(void*) == 8, \"should be false\");",
        "struct ax { char a; }; _Static_assert(sizeof(struct ax) == 1, \"\");",
        "struct ax { char a; int b; }; _Static_assert(sizeof(struct ax) == 8, \"\");",
        "struct ax { int a:10; int b:20; int c; }; _Static_assert(sizeof(struct ax) == 8, \"\");",
        "struct ax { int a:1; int c; }; _Static_assert(sizeof(struct ax) == 8, \"\");",
        "struct ax { int a:10; int b:20; int c:2; int d; }; _Static_assert(sizeof(struct ax) == 8, \"\");",
        "struct ax { int a:1; int c; int d:1; }; _Static_assert(sizeof(struct ax) == 12, \"\");",
        "int main() {}",
        "int main(int argc, char* argv[]) {}",
        "int main(int argc, char** argv) {}",
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
    EENTRY("_Static_assert(sizeof(void*) == 0, \"should be false\");", StaticAssertFailed) \
    EENTRY("int a; double a;", Redefinition) \
    EENTRY("struct ax {}; _Static_assert(sizeof(struct ax) == 0, \"\");", EmptyStructUnionDefinition) \
    EENTRY("union  ax {}; _Static_assert(sizeof(union ax) == 0, \"\");", EmptyStructUnionDefinition) \
    EENTRY("int main(int argc, char** argv, void) {}", InvalidFunctionParameter) \
    // EENTRY("int a; double a;", None) \

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
