
## SimpleCalculator Grammar

``` c
/** Syntax

  CaclUnit         : Statement
  CaclUnit         : FunctionDef
  CaclUnit         : CaclUnit Statement
  CaclUnit         : CaclUnit FunctionDef

  ArgList          : ArgList Comma ID
  ArgList          : ID

  FunctionDef      : 'function' LPAREN RPAREN BlockStatement
  FunctionDef      : 'function' LPAREN ArgList RPAREN BlockStatement

  IfStatement      : 'if' LPAREN Expr RPAREN BlockStatement 'else' BlockStatement
  IfStatement      : 'if' LPAREN Expr RPAREN BlockStatement

  ForStatement     : 'for' LPAREN ExprList Semicolon Expr Semicolon ExprList RPAREN BlockStatement
  
  WhileStatement   : 'while' LPAREN Expr LPAREN BlockStatement

  ReturnStatement  : 'return' Expr Semicolon

  StatementList    : Statement
  StatementList    : StatementList, Statement

  BlockStatement   : LBRACE LBRACE
  BlockStatement   : LBRACE StatementList LBRACE

  Statement        : Semicolon
  Statement        : ExprList Semicolon
  Statement        : IfStatement
  Statement        : ForStatement
  Statement        : WhileStatement
  Statement        : ReturnStatement
  Statement        : BlockStatement

  ExprList         : ExprList Comma Expr
  ExprList         : Expr

  Expr             : ID | NUMBER
  Expr             : LPAREN Expr LPAREN
  Expr             : Expr '+' Expr
  Expr             : Expr '-' Expr
  Expr             : Expr '*' Expr
  Expr             : Expr '/' Expr
  Expr             : Expr '%' Expr
  Expr             : Expr '^' Expr
  Expr             : Expr '=' Expr
  Expr             : Expr '==' Expr
  Expr             : Expr '!=' Expr
  Expr             : Expr '>' Expr
  Expr             : Expr '<' Expr
  Expr             : Expr '>=' Expr
  Expr             : Expr '<=' Expr
  Expr             : '++' Expr
  Expr             : '--' Expr
  Expr             : Expr '++'
  Expr             : Expr '--'

 */
```
