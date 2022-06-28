## DCParse

从头实现一个C99编译器

### 项目概述

* [x] [正则表达式](./include/regex/). 通过正则表达式构建 DFA, 实现不匹配的字符串可以不需要全部输入就可判定为不匹配
* [x] [Tokenizer](./include//lexer/) 利用正则表达式或者其他规则将文本转换为 tokens
* [x] [Parser](./include//parser/) Bottom-Up parser, 并实现额外的规则解决 `typedef` 导致的问题
* [x] [Calculator](./example/SimpleCalculator/) 利用以上工具实现的一个简单的计算器, 可以定义函数并有一些预置的函数
* [x] [cparser](./cparser/) C99编译器(未完成), 目标代码为 (WASM)
  * [x] 语法规则
  * [x] 类型检查
  * [ ] 代码生成
  * [ ] 优化