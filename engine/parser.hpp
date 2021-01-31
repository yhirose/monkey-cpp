#pragma once

#include <ast.hpp>
#include <sstream>

namespace monkey {

const auto GRAMMAR = R"(

PROGRAM                <-  STATEMENTS

STATEMENTS             <-  (STATEMENT ';'?)*
STATEMENT              <-  ASSIGNMENT / RETURN / EXPRESSION_STATEMENT

ASSIGNMENT             <-  'let' IDENTIFIER '=' EXPRESSION
RETURN                 <-  'return' EXPRESSION
EXPRESSION_STATEMENT   <-  EXPRESSION

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

IDENTIFIER             <-  < [a-zA-Z]+ >
INTEGER                <-  < [0-9]+ >
STRING                 <-  < ["] < (!["] .)* > ["] >
BOOLEAN                <-  'true' / 'false'
NULL                   <-  'null'
PREFIX_OPE             <-  < [-!] >
INFIX_OPE              <-  < [-+/*<>] / '==' / '!=' >

KEYWORD                <-  'null' | 'true' | 'false' | 'let' | 'return' | 'if' | 'else' | 'fn'

LIST(ITEM, DELM)       <-  (ITEM (~DELM ITEM)*)?

LINE_COMMENT           <-  '//' (!LINE_END .)* &LINE_END
LINE_END               <-  '\r\n' / '\r' / '\n' / !.

%whitespace            <-  ([ \t\r\n]+ / LINE_COMMENT)*
%word                  <-  [a-zA-Z]+

)";

inline peg::parser &get_parser() {
  static peg::parser parser;
  static bool initialized = false;

  if (!initialized) {
    initialized = true;

    parser.log = [&](size_t ln, size_t col, const std::string &msg) {
      std::cerr << ln << ":" << col << ": " << msg << std::endl;
    };

    if (!parser.load_grammar(GRAMMAR)) {
      throw std::logic_error("invalid peg grammar");
    }

    parser.enable_ast<Ast>();
  }

  return parser;
}

inline std::shared_ptr<Ast> parse(const std::string &path, const char *expr,
                                  size_t len, std::vector<std::string> &msgs) {
  auto &parser = get_parser();

  parser.log = [&](size_t ln, size_t col, const std::string &err_msg) {
    std::stringstream ss;
    ss << path << ":" << ln << ":" << col << ": " << err_msg << std::endl;
    msgs.push_back(ss.str());
  };

  std::shared_ptr<Ast> ast;
  if (parser.parse_n(expr, len, ast, path.c_str())) {
    auto opt =
        peg::AstOptimizer(true, {"EXPRESSION_STATEMENT", "PARAMETERS", "ARGUMENTS",
                                 "INDEX", "RETURN", "BLOCK", "ARRAY", "HASH"});

    ast = opt.optimize(ast);
    annotate(ast);
    return ast;
  }

  return nullptr;
}

} // namespace monkey
