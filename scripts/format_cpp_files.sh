#!/bin/bash

# exclude build directory and third_party directory
find cparser include example test lib -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.hpp" -o -name "*.hxx" | xargs clang-format -i