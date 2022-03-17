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
        "int f1 = 1;",
        "int f1 = 1.1;",

        "char a[] = \"helloworld\";",
        "char a[][8] = { \"helloworld\", \"1234\" };",
        "char a[] = { \"helloworld\" };",

        "void test1() {\n"
        "    struct s1 { struct s2 { int a; int b; } v2; struct s3 { void* a; int   b; } v3;};\n"
        "    struct s1 s1; struct s2 s2; struct s3 s3;\n"
        "    struct s1 val[] = { 1, 2, { }, { }, s2, s3, { } };\n"
        "    _Static_assert(sizeof(val) / sizeof(*val) == 4, \"\");"
        "}",

        "void test2() {\n"
        "    struct s1 { struct s2 { int a; int b; struct s5 { float a; float b; } c; } v2; struct s3 { void* a; int   b; } v3; };\n"
        "    struct s1 s1; struct s2 s2; struct s3 s3;\n"
        "    struct s1 val[] = { 1, 2, { }, { }, s2, s3,  { } };\n"
        "    _Static_assert(sizeof(val) / sizeof(*val) == 3, \"\");"
        "}",

        "void test3()\n"
        "{\n"
        "   struct s1 { struct s2 {} a1; int b1; struct s3 { struct s4 { } n1; int a, b, c; } a2; int* c1, c2; };\n"
        "   struct s1 s1; struct s2 s2; struct s3 s3; struct s4 s4;\n"
        "   struct s1 val1[] = { [4].b1 = 1, s4, 1, 2, 3, (void*)0 };\n"
        "   struct s1 val2[] = { [0].b1 = 1, {}, (void*)0 };\n"
        "   _Static_assert(sizeof(val1) / sizeof(*val1) == 5, \"\");\n"
        "   _Static_assert(sizeof(val2) / sizeof(*val2) == 1, \"\");\n"
        "}\n",
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

        auto errors = reporter->get_errors();
        auto warnings = reporter->get_warnings();
        auto infos = reporter->get_infos();
        for (auto msg: errors)
            cerr << msg->what() << endl;
        for (auto msg: warnings)
            cerr << msg->what() << endl;
        for (auto msg: infos)
            cerr << msg->what() << endl;
        ASSERT_EQ(reporter->error_count(), 0) << t;
        parser.reset();
    }
}

TEST(should_reject, CAST) {
    CLexerParser parser;

#define EXPECT_REJECT_LIST \
    EENTRY("_Static_assert(0, \"hello, this is a static_assert failure\");", StaticAssertFailed) \
    EENTRY("_Static_assert(sizeof(void*) == 0, \"should be false\");", StaticAssertFailed) \
    EENTRY("int a; double a;", Redefinition) \
    EENTRY("int main(int argc, char** argv, void) {}", InvalidFunctionParameter) \
    EENTRY("int f1 = \"value\";", InvalidOperand) \
    // EENTRY("int a; double a;", None) \

#define EXPECT_WARNING_LIST \
    EENTRY("struct ax {}; _Static_assert(sizeof(struct ax) == 0, \"\");", EmptyStructUnionDefinition) \
    EENTRY("union  ax {}; _Static_assert(sizeof(union ax) == 0, \"\");", EmptyStructUnionDefinition) \

    shared_ptr<SemanticError> error, warning, info;
    const auto eval_code = [&](string code) {
        error = warning = info = nullptr;
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

        auto errors = reporter->get_errors();
        auto warnings = reporter->get_warnings();
        auto infos = reporter->get_infos();
        if (errors.size() > 0)
            error = errors[0];
        if (warnings.size() > 0)
            warning = warnings[0];
        if (infos.size() > 0)
            info = infos[0];

        parser.reset();
    };


#define EENTRY(str, type) \
    { \
        eval_code(str); \
        ASSERT_NE(error, nullptr) << str; \
        auto serr = dynamic_pointer_cast<SemanticError##type>(error); \
        ASSERT_NE(serr, nullptr) << str << endl << "    " << error->what(); \
    }
EXPECT_REJECT_LIST
#undef EENTRY

#define EENTRY(str, type) \
    { \
        eval_code(str); \
        ASSERT_EQ(error, nullptr) << str; \
        ASSERT_NE(warning, nullptr) << str; \
        auto swarning = dynamic_pointer_cast<SemanticError##type>(warning); \
        ASSERT_NE(swarning, nullptr) << str << endl << "    " << warning->what(); \
    }
EXPECT_WARNING_LIST
#undef EENTRY
}
