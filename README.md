# monkey

Another implementation of Monkey programming language described in [Writing An Interpreter In Go](https://interpreterbook.com/) and [Writing A Compiler In Go](https://compilerbook.com/).
This is written in C++ and uses [cpp-peglib](https://github.com/yhirose/cpp-peglib) PEG library for lexter and parser.

In addition to the original Monkey language spec, this implementation supports the line comment. Macro system in Appendix A is not implemented, yet.

## Install

```bash
$ git checkout https://github.com/yhirose/monkey-cpp.git
$ cd monkey-cpp
$ make

$ ./build/monkey
>> puts("hello " + "world!")
hello world!
null
>> quit

$ ./build/monkey examples/map.monkey
[2, 4, 6, 8]
15
```

## PEG grammar

```
PROGRAM                <-  STATEMENTS

STATEMENTS             <-  (STATEMENT ';'?)*
STATEMENT              <-  ASSIGNMENT / RETURN / EXPRESSION

ASSIGNMENT             <-  'let' IDENTIFIER '=' EXPRESSION
RETURN                 <-  'return' EXPRESSION

EXPRESSION             <-  INFIX_EXPR(PREFIX_EXPR, INFIX_OPE)
INFIX_EXPR(ATOM, OPE)  <-  ATOM (OPE ATOM)* {
                             precedence
                               L == !=
                               L < >
                               L + -
                               L * /
                           }

IF                     <-  'if' '(' EXPRESSION ')' BLOCK ('else' BLOCK)?

FUNCTION               <-  'fn' '(' PARAMETERS ')' BLOCK
PARAMETERS             <-  LIST(IDENTIFIER, ',')

BLOCK                  <-  '{' STATEMENTS '}'

CALL                   <-  PRIMARY (ARGUMENTS / INDEX)*
ARGUMENTS              <-  '(' LIST(EXPRESSION, ',') ')'
INDEX                  <-   '[' EXPRESSION ']'

PREFIX_EXPR            <-  PREFIX_OPE* CALL
PRIMARY                <-  IF / FUNCTION / ARRAY / HASH / INTEGER / BOOLEAN / NULL / IDENTIFIER / STRING / '(' EXPRESSION ')'

ARRAY                  <-  '[' LIST(EXPRESSION, ',') ']'

HASH                   <-  '{' LIST(HASH_PAIR, ',') '}'
HASH_PAIR              <-  EXPRESSION ':' EXPRESSION

IDENTIFIER             <-  !KEYWORD < [a-zA-Z]+ >
INTEGER                <-  < [0-9]+ >
STRING                 <-  < ["] < (!["] .)* > ["] >
BOOLEAN                <-  < 'true' / 'false' >
NULL                   <-  'null'
PREFIX_OPE             <-  < [-!] >
INFIX_OPE              <-  < [-+/*<>] / '==' / '!=' >

KEYWORD                <-  'null' | 'true' | 'false' | 'let' | 'return' | 'if' | 'else' | 'fn'

LIST(ITEM, DELM)       <-  (ITEM (~DELM ITEM)*)?

LINE_COMMENT           <-  '//' (!LINE_END .)* &LINE_END
LINE_END               <-  '\r\n' / '\r' / '\n' / !.
%whitespace            <-  ([ \t\r\n]+ / LINE_COMMENT)*
```
