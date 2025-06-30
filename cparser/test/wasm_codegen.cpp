#include "wasm_codegen.h"
#include "c_lexer_parser.h"
#include "c_reporter.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace cparser;
using namespace std;

class WasmCodeGenTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        codegen = std::make_unique<WasmCodeGenerator>();
        parser = std::make_unique<CLexerParser>();
    }

    void TearDown() override
    {
        codegen.reset();
        parser.reset();
    }

    // Helper function to parse C code and return AST
    std::shared_ptr<ASTNodeTranslationUnit> parseCode(const std::string& code)
    {
        parser->reset();
        for (char c : code) {
            parser->feed(c);
        }
        return parser->end();
    }

    // Helper function to parse and generate WASM module
    std::string generateWasmModule(const std::string& code)
    {
        auto ast = parseCode(code);
        EXPECT_NE(ast, nullptr) << "Failed to parse code: " << code;
        if (!ast)
            return "";

        auto reporter = std::make_shared<SemanticReporter>();
        ast->check_constraints(reporter);
        EXPECT_EQ(reporter->error_count(), 0) << "Semantic errors in code: " << code;
        for (auto& e : *reporter) {
            std::cout << e->error_message() << std::endl;
        }

        return codegen->generateModule(ast);
    }

    std::unique_ptr<WasmCodeGenerator> codegen;
    std::unique_ptr<CLexerParser> parser;
};

// Test basic type mapping
TEST_F(WasmCodeGenTest, TypeMapping)
{
    // Test integer function
    std::string code = "int add(int a, int b) { return a + b; }";
    std::string wasm = generateWasmModule(code);

    EXPECT_FALSE(wasm.empty());
    EXPECT_NE(wasm.find("(func $add"), std::string::npos);
    EXPECT_NE(wasm.find("(param $a i32)"), std::string::npos);
    EXPECT_NE(wasm.find("(param $b i32)"), std::string::npos);
    EXPECT_NE(wasm.find("(result i32)"), std::string::npos);
}

// Test float function
TEST_F(WasmCodeGenTest, FloatFunction)
{
    std::string code = "float multiply(float x, float y) { return x * y; }";
    std::string wasm = generateWasmModule(code);

    EXPECT_FALSE(wasm.empty());
    EXPECT_NE(wasm.find("(func $multiply"), std::string::npos);
    EXPECT_NE(wasm.find("(param $x f32)"), std::string::npos);
    EXPECT_NE(wasm.find("(param $y f32)"), std::string::npos);
    EXPECT_NE(wasm.find("(result f32)"), std::string::npos);
}

// Test void function
TEST_F(WasmCodeGenTest, VoidFunction)
{
    std::string code = "void test() { return; }";
    std::string wasm = generateWasmModule(code);

    EXPECT_FALSE(wasm.empty());
    EXPECT_NE(wasm.find("(func $test"), std::string::npos);
    EXPECT_EQ(wasm.find("(result"), std::string::npos); // No result type for void
}

// Test arithmetic operations
TEST_F(WasmCodeGenTest, ArithmeticOperations)
{
    std::string code = R"(
        int arithmetic_test(int a, int b) {
            int sum = a + b;
            int diff = a - b;
            int prod = a * b;
            int quot = a / b;
            int rem = a % b;
            return sum + diff * prod - quot + rem;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Check for arithmetic instructions
    EXPECT_NE(wasm.find("i32.add"), std::string::npos);
    EXPECT_NE(wasm.find("i32.sub"), std::string::npos);
    EXPECT_NE(wasm.find("i32.mul"), std::string::npos);
    EXPECT_NE(wasm.find("i32.div_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.rem_s"), std::string::npos);
}

// Test comparison operations
TEST_F(WasmCodeGenTest, ComparisonOperations)
{
    std::string code = R"(
        int compare_test(int a, int b) {
            if (a < b) return 1;
            if (a <= b) return 2;
            if (a > b) return 3;
            if (a >= b) return 4;
            if (a == b) return 5;
            if (a != b) return 6;
            return 0;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Check for comparison instructions
    EXPECT_NE(wasm.find("i32.lt_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.le_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.gt_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.ge_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.eq"), std::string::npos);
    EXPECT_NE(wasm.find("i32.ne"), std::string::npos);
}

// Test logical operations
TEST_F(WasmCodeGenTest, LogicalOperations)
{
    std::string code = R"(
        int logical_test(int a, int b) {
            if (a && b) return 1;
            if (a || b) return 2;
            if (!a) return 3;
            return 0;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Logical operations should be implemented with conditional jumps
    EXPECT_NE(wasm.find("br_if"), std::string::npos);
}

// Test bitwise operations
TEST_F(WasmCodeGenTest, BitwiseOperations)
{
    std::string code = R"(
        int bitwise_test(int a, int b) {
            int and_result = a & b;
            int or_result = a | b;
            int xor_result = a ^ b;
            int not_result = ~a;
            int left_shift = a << 2;
            int right_shift = a >> 2;
            return and_result + or_result + xor_result + not_result + left_shift + right_shift;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Check for bitwise instructions
    EXPECT_NE(wasm.find("i32.and"), std::string::npos);
    EXPECT_NE(wasm.find("i32.or"), std::string::npos);
    EXPECT_NE(wasm.find("i32.xor"), std::string::npos);
    EXPECT_NE(wasm.find("i32.shl"), std::string::npos);
    EXPECT_NE(wasm.find("i32.shr_s"), std::string::npos);
}

// Test unary operations
TEST_F(WasmCodeGenTest, UnaryOperations)
{
    std::string code = R"(
        int unary_test(int a) {
            int neg = -a;
            int pos = +a;
            int pre_inc = ++a;
            int post_inc = a++;
            int pre_dec = --a;
            int post_dec = a--;
            return neg + pos + pre_inc + post_inc + pre_dec + post_dec;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Negation should generate subtraction from zero
    EXPECT_NE(wasm.find("i32.sub"), std::string::npos);
}

// Test if statement
TEST_F(WasmCodeGenTest, IfStatement)
{
    std::string code = R"(
        int if_test(int a) {
            if (a > 0) {
                return 1;
            } else {
                return -1;
            }
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    std::cout << "Generated WASM for IfStatement test:\n" << wasm << std::endl;

    // Should contain conditional branching
    EXPECT_NE(wasm.find("if"), std::string::npos);
    EXPECT_NE(wasm.find("else"), std::string::npos);
}

// Test for loop
TEST_F(WasmCodeGenTest, ForLoop)
{
    std::string code = R"(
        int for_loop_test() {
            int sum = 0;
            for (int i = 0; i < 10; i++) {
                sum += i;
            }
            return sum;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain loop constructs
    EXPECT_NE(wasm.find("loop"), std::string::npos);
    EXPECT_NE(wasm.find("br_if"), std::string::npos);
}

// Test while loop
TEST_F(WasmCodeGenTest, WhileLoop)
{
    std::string code = R"(
        int while_loop_test(int n) {
            int count = 0;
            while (n > 0) {
                count++;
                n--;
            }
            return count;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain loop constructs
    EXPECT_NE(wasm.find("loop"), std::string::npos);
    EXPECT_NE(wasm.find("br_if"), std::string::npos);
}

// Test function calls
TEST_F(WasmCodeGenTest, FunctionCalls)
{
    std::string code = R"(
        int helper(int x) {
            return x * 2;
        }
        
        int caller(int a) {
            return helper(a) + helper(a + 1);
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain function calls
    EXPECT_NE(wasm.find("call $helper"), std::string::npos);
}

// Test local variables
TEST_F(WasmCodeGenTest, LocalVariables)
{
    std::string code = R"(
        int local_vars_test(int param) {
            int local1 = param + 1;
            int local2 = local1 * 2;
            return local1 + local2;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain local variable operations
    EXPECT_NE(wasm.find("local.get"), std::string::npos);
    EXPECT_NE(wasm.find("local.set"), std::string::npos);
}

// Test global variables
TEST_F(WasmCodeGenTest, GlobalVariables)
{
    std::string code = R"(
        int global_var = 42;
        
        int get_global() {
            return global_var;
        }
        
        void set_global(int value) {
            global_var = value;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain global variable operations
    EXPECT_NE(wasm.find("global"), std::string::npos);
}

// Test pointers and arrays (basic)
TEST_F(WasmCodeGenTest, PointersAndArrays)
{
    std::string code = R"(
        int array_test() {
            int arr[5] = {1, 2, 3, 4, 5};
            int *ptr = arr;
            return *ptr + *(ptr + 1);
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain memory operations
    EXPECT_NE(wasm.find("i32.load"), std::string::npos);
}

// Test string literals
TEST_F(WasmCodeGenTest, StringLiterals)
{
    std::string code = R"(
        const char* get_string() {
            return "Hello, World!";
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain data section for string
    EXPECT_NE(wasm.find("data"), std::string::npos);
}

// Test struct definitions (basic)
TEST_F(WasmCodeGenTest, BasicStructs)
{
    std::string code = R"(
        struct Point {
            int x;
            int y;
        };
        
        int get_x(struct Point p) {
            return p.x;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Struct access should generate memory operations
    EXPECT_NE(wasm.find("i32.load"), std::string::npos);
}

// Test type casting
TEST_F(WasmCodeGenTest, TypeCasting)
{
    std::string code = R"(
        float int_to_float(int x) {
            return (float)x;
        }
        
        int float_to_int(float x) {
            return (int)x;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain type conversion instructions
    EXPECT_NE(wasm.find("f32.convert_i32_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.trunc_f32_s"), std::string::npos);
}

// Test complex expressions
TEST_F(WasmCodeGenTest, ComplexExpressions)
{
    std::string code = R"(
        int complex_expr(int a, int b, int c) {
            return (a + b) * (c - a) / (b + c) + (a * b * c) % (a + b + c);
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain multiple arithmetic operations
    EXPECT_NE(wasm.find("i32.add"), std::string::npos);
    EXPECT_NE(wasm.find("i32.sub"), std::string::npos);
    EXPECT_NE(wasm.find("i32.mul"), std::string::npos);
    EXPECT_NE(wasm.find("i32.div_s"), std::string::npos);
    EXPECT_NE(wasm.find("i32.rem_s"), std::string::npos);
}

// Test nested control flow
TEST_F(WasmCodeGenTest, NestedControlFlow)
{
    std::string code = R"(
        int nested_test(int n) {
            int result = 0;
            for (int i = 0; i < n; i++) {
                if (i % 2 == 0) {
                    for (int j = 0; j < i; j++) {
                        result += j;
                    }
                } else {
                    result -= i;
                }
            }
            return result;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain nested loop and conditional structures
    EXPECT_NE(wasm.find("loop"), std::string::npos);
    EXPECT_NE(wasm.find("if"), std::string::npos);
    EXPECT_NE(wasm.find("br_if"), std::string::npos);
}

// Test error handling for invalid input
TEST_F(WasmCodeGenTest, ErrorHandling)
{
    // Test with null AST
    EXPECT_NO_THROW({
        std::string result = codegen->generateModule(nullptr);
        EXPECT_TRUE(result.empty());
    });

    // Test module generation with empty function
    std::string code = "void empty_func() {}";
    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());
    EXPECT_NE(wasm.find("(func $empty_func"), std::string::npos);
}

// Test module structure
TEST_F(WasmCodeGenTest, ModuleStructure)
{
    std::string code = R"(
        int main() {
            return 42;
        }
    )";

    std::string wasm = generateWasmModule(code);
    EXPECT_FALSE(wasm.empty());

    // Should contain proper module structure
    EXPECT_NE(wasm.find("(module"), std::string::npos);
    EXPECT_NE(wasm.find("(memory"), std::string::npos);
    EXPECT_NE(wasm.find("(func $main"), std::string::npos);
    EXPECT_NE(wasm.find("(export \"main\""), std::string::npos);
}

// Test reset functionality
TEST_F(WasmCodeGenTest, ResetFunctionality)
{
    std::string code1 = "int func1() { return 1; }";
    std::string code2 = "int func2() { return 2; }";

    std::string wasm1 = generateWasmModule(code1);
    codegen->reset();
    std::string wasm2 = generateWasmModule(code2);

    EXPECT_FALSE(wasm1.empty());
    EXPECT_FALSE(wasm2.empty());
    EXPECT_NE(wasm1.find("func1"), std::string::npos);
    EXPECT_NE(wasm2.find("func2"), std::string::npos);
    EXPECT_EQ(wasm2.find("func1"), std::string::npos); // func1 should not be in wasm2
}
