#include "catch.hh"
#include "test-util.hpp"

using namespace std;
using namespace peg::udl;
using namespace monkey;

void testIntegerLiteral(const shared_ptr<Ast> &ast, int64_t number) {
  CHECK(ast->name == "INTEGER");
  CHECK(any_cast<int64_t>(ast->value) == number);
}

void testIdentifier(const shared_ptr<Ast> &ast, const string &token) {
  CHECK(ast->name == "IDENTIFIER");
  CHECK(ast->token == token);
}

void testBooleanLiteral(const shared_ptr<Ast> &ast, int64_t value) {
  CHECK(ast->name == "BOOLEAN");
  CHECK(any_cast<bool>(ast->value) == value);
}

void testStringLiteral(const shared_ptr<Ast> &ast, const char *token) {
  CHECK(ast->name == "STRING");
  CHECK(ast->token == token);
}

void testLiteralExpression(const shared_ptr<Ast> &ast, any value) {
  switch (ast->tag) {
  case "INTEGER"_: testIntegerLiteral(ast, any_cast<int64_t>(value)); break;
  case "IDENTIFIER"_: testIdentifier(ast, any_cast<const char *>(value)); break;
  case "BOOLEAN"_: testBooleanLiteral(ast, any_cast<bool>(value)); break;
  }
}

void testInfixExpression(const shared_ptr<Ast> &ast, any leftValue,
                         const string &operatorToken, any rightValue) {
  CHECK(ast->name == "INFIX_EXPR");

  testLiteralExpression(ast->nodes[0], leftValue);

  CHECK(ast->nodes[1]->name == "INFIX_OPE");
  CHECK(ast->nodes[1]->token == operatorToken);

  testLiteralExpression(ast->nodes[2], rightValue);
}

TEST_CASE("'let' statements", "[parser]") {
  struct Test {
    string input;
    string expectedIdentifier;
    any expectedValue;
  };

  Test tests[] = {
      {"let x = 5;", "x", int64_t(5)},
      {"let y = true;", "y", true},
      {"let foobar = y;", "foobar", "y"},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: 'let' statements)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "ASSIGNMENT");

    testIdentifier(ast->nodes[0], t.expectedIdentifier);
    testLiteralExpression(ast->nodes[1], t.expectedValue);
  }
}

TEST_CASE("'return' statements", "[parser]") {
  struct Test {
    string input;
    any expectedValue;
  };

  Test tests[] = {
      {"return 5;", int64_t(5)},
      {"return true;", true},
      {"return foobar;", "foobar"},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: 'return' statements)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "RETURN");

    testLiteralExpression(ast->nodes[0], t.expectedValue);
  }
}

TEST_CASE("Identifier expression", "[parser]") {
  auto ast = parse("([parser]: Identifier expression)", "foobar;");
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  testIdentifier(ast->nodes[0], "foobar");
}

TEST_CASE("Integer literal expression", "[parser]") {
  auto ast = parse("([parser]: Integer literal expression)", "5;");
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  testIntegerLiteral(ast->nodes[0], 5);
}

TEST_CASE("Parsing prefix expression", "[parser]") {
  struct Test {
    string input;
    string operatorToken;
    any value;
  };

  Test tests[] = {
      {"!5;", "!", int64_t(5)},    {"-15;", "-", int64_t(15)},
      {"!foobar;", "!", "foobar"}, {"-foobar;", "-", "foobar"},
      {"!true;", "!", true},       {"!false;", "!", false},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Parsing prefix expression)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "EXPRESSION_STATEMENT");

    auto node = ast->nodes[0];
    REQUIRE(node->name == "PREFIX_EXPR");

    CHECK(node->nodes[0]->name == "PREFIX_OPE");
    CHECK(node->nodes[0]->token == t.operatorToken);

    testLiteralExpression(node->nodes[1], t.value);
  }
}

TEST_CASE("Parsing infix expression", "[parser]") {
  struct Test {
    string input;
    any leftValue;
    string operatorToken;
    any rightValue;
  };

  Test tests[] = {
      {"5 + 5;", int64_t(5), "+", int64_t(5)},
      {"5 - 5;", int64_t(5), "-", int64_t(5)},
      {"5 * 5;", int64_t(5), "*", int64_t(5)},
      {"5 / 5;", int64_t(5), "/", int64_t(5)},
      {"5 < 5;", int64_t(5), "<", int64_t(5)},
      {"5 > 5;", int64_t(5), ">", int64_t(5)},
      {"5 == 5;", int64_t(5), "==", int64_t(5)},
      {"5 != 5;", int64_t(5), "!=", int64_t(5)},
      {"foobar + barfoo;", "foobar", "+", "barfoo"},
      {"foobar - barfoo;", "foobar", "-", "barfoo"},
      {"foobar * barfoo;", "foobar", "*", "barfoo"},
      {"foobar / barfoo;", "foobar", "/", "barfoo"},
      {"foobar < barfoo;", "foobar", "<", "barfoo"},
      {"foobar > barfoo;", "foobar", ">", "barfoo"},
      {"foobar == barfoo;", "foobar", "==", "barfoo"},
      {"foobar != barfoo;", "foobar", "!=", "barfoo"},
      {"true == true;", true, "==", true},
      {"true != false;", true, "!=", false},
      {"false == false;", false, "==", false},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Parsing infix expression)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "EXPRESSION_STATEMENT");

    testInfixExpression(ast->nodes[0], t.leftValue, t.operatorToken, t.rightValue);
  }
}

TEST_CASE("Operator precedence parsing", "[parser]") {
  struct Test {
    string input;
    string expected;
  };

  Test tests[] = {
      {
          "-a * b",
          "((-a) * b)",
      },
      {
          "!-a",
          "(!(-a))",
      },
      {
          "a + b + c",
          "((a + b) + c)",
      },
      {
          "a + b - c",
          "((a + b) - c)",
      },
      {
          "a * b * c",
          "((a * b) * c)",
      },
      {
          "a * b / c",
          "((a * b) / c)",
      },
      {
          "a + b / c",
          "(a + (b / c))",
      },
      {
          "a + b * c + d / e - f",
          "(((a + (b * c)) + (d / e)) - f)",
      },
      {
          "3 + 4; -5 * 5",
          "(3 + 4)((-5) * 5)",
      },
      {
          "5 > 4 == 3 < 4",
          "((5 > 4) == (3 < 4))",
      },
      {
          "5 < 4 != 3 > 4",
          "((5 < 4) != (3 > 4))",
      },
      {
          "3 + 4 * 5 == 3 * 1 + 4 * 5",
          "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",
      },
      {
          "true",
          "true",
      },
      {
          "false",
          "false",
      },
      {
          "3 > 5 == false",
          "((3 > 5) == false)",
      },
      {
          "3 < 5 == true",
          "((3 < 5) == true)",
      },
      {
          "1 + (2 + 3) + 4",
          "((1 + (2 + 3)) + 4)",
      },
      {
          "(5 + 5) * 2",
          "((5 + 5) * 2)",
      },
      {
          "2 / (5 + 5)",
          "(2 / (5 + 5))",
      },
      {
          "(5 + 5) * 2 * (5 + 5)",
          "(((5 + 5) * 2) * (5 + 5))",
      },
      {
          "-(5 + 5)",
          "(-(5 + 5))",
      },
      {
          "!(true == true)",
          "(!(true == true))",
      },
      {
          "a + add(b * c) + d",
          "((a + add((b * c))) + d)",
      },
      {
          "add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))",
          "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))",
      },
      {
          "add(a + b + c * d / f + g)",
          "add((((a + b) + ((c * d) / f)) + g))",
      },
      {
          "a * [1, 2, 3, 4][b * c] * d",
          "((a * ([1, 2, 3, 4][(b * c)])) * d)",
      },
      {
          "a * [1, 2, 3, 4][b * c][0] * d",
          "((a * (([1, 2, 3, 4][(b * c)])[0])) * d)",
      },
      {
          "add(a * b[2], b[1], 2 * [1, 2][1])",
          "add((a * (b[2])), (b[1]), (2 * ([1, 2][1])))",
      },
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Operator precedence parsing)", t.input);

    REQUIRE(ast != nullptr);
    REQUIRE(to_string(ast) == t.expected);
  }
}

TEST_CASE("Boolean expression", "[parser]") {
  struct Test {
    string input;
    bool expectedBoolean;
  };

  Test tests[] = {
      {"true;", true},
      {"false;", false},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Boolean expression)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "EXPRESSION_STATEMENT");

    testBooleanLiteral(ast->nodes[0], t.expectedBoolean);
  }
}

TEST_CASE("If expression", "[parser]") {
  auto ast = parse("([parser]: If expression)", "if (x < y) { x };");
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "IF");

  testInfixExpression(node->nodes[0], "x", "<", "y");
  testIdentifier(node->nodes[1]->nodes[0]->nodes[0], "x");
}

TEST_CASE("If else expression", "[parser]") {
  auto ast =
      parse("([parser]: If else expression)", "if (x < y) { x } else { y };");
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "IF");

  testInfixExpression(node->nodes[0], "x", "<", "y");
  testIdentifier(node->nodes[1]->nodes[0]->nodes[0], "x");
  testIdentifier(node->nodes[2]->nodes[0]->nodes[0], "y");
}

TEST_CASE("Function literal parsing", "[parser]") {
  auto ast =
      parse("([parser]: Function literal parser)", "fn(x, y) { x + y; }");
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "FUNCTION");

  testIdentifier(node->nodes[0]->nodes[0], "x");
  testIdentifier(node->nodes[0]->nodes[1], "y");

  {
    const auto& node2 = node->nodes[1]->nodes[0];
    REQUIRE(node2->name == "EXPRESSION_STATEMENT");

    testInfixExpression(node2->nodes[0], "x", "+", "y");
  }
}

TEST_CASE("Function parameter parsing", "[parser]") {
  struct Test {
    string input;
    vector<string> expectedParams;
  };

  Test tests[] = {
      {"fn() {};", {}},
      {"fn(x) {};", {"x"}},
      {"fn(x, y, z) {};", {"x", "y", "z"}},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Function parameter parsing)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "EXPRESSION_STATEMENT");

    auto node = ast->nodes[0];
    REQUIRE(node->name == "FUNCTION");

    auto nodes = node->nodes[0]->nodes;
    CHECK(nodes.size() == t.expectedParams.size());

    for (size_t i = 0; i < nodes.size(); i++) {
      testIdentifier(nodes[i], t.expectedParams[i]);
    }
  }
}

TEST_CASE("Call expression parsing", "[parser]") {
  auto input = "add(1, 2 * 3, 4 + 5);";
  auto ast = parse("([parser]: Call expression parsing)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "CALL");

  testIdentifier(node->nodes[0], "add");

  REQUIRE(node->nodes[1]->name == "ARGUMENTS");
  auto nodes = node->nodes[1]->nodes;
  testLiteralExpression(nodes[0], int64_t(1));
  testInfixExpression(nodes[1], int64_t(2), "*", int64_t(3));
  testInfixExpression(nodes[2], int64_t(4), "+", int64_t(5));
}

TEST_CASE("Call expression parameter parsing", "[parser]") {
  struct Test {
    string input;
    string expectedIdent;
    vector<string> expectedArgs;
  };

  Test tests[] = {
      {"add();", "add", {}},
      {"add(1);", "add", {"1"}},
      {"add(1, 2 * 3, 4 + 5);", "add", {"1", "(2 * 3)", "(4 + 5)"}},
  };

  for (const auto &t : tests) {
    auto ast = parse("([parser]: Call expression parameter parsing)", t.input);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->name == "EXPRESSION_STATEMENT");

    auto node = ast->nodes[0];
    REQUIRE(node->name == "CALL");

    testIdentifier(node->nodes[0], t.expectedIdent);

    REQUIRE(node->nodes[1]->name == "ARGUMENTS");
    auto nodes = node->nodes[1]->nodes;
    CHECK(nodes.size() == t.expectedArgs.size());

    for (size_t i = 0; i < nodes.size(); i++) {
      CHECK(to_string(nodes[i]) == t.expectedArgs[i]);
    }
  }
}

TEST_CASE("String literal expression", "[parser]") {
  auto input = R"("hello world";)";
  auto ast = parse("([parser]: String literal expression)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "STRING");

  testStringLiteral(node, "hello world");
}

TEST_CASE("Parsing empty array literals", "[parser]") {
  auto input = "[]";
  auto ast = parse("([parser]: Parsing empty array literals)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "ARRAY");

  CHECK(node->nodes.empty());
}

TEST_CASE("Parsing array literals", "[parser]") {
  auto input = "[1, 2 * 2, 3 + 3]";
  auto ast = parse("([parser]: Parsing array literals)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "ARRAY");

  auto nodes = node->nodes;
  CHECK(nodes.size() == 3);

  testIntegerLiteral(nodes[0], 1);
  testInfixExpression(nodes[1], int64_t(2), "*", int64_t(2));
  testInfixExpression(nodes[2], int64_t(3), "+", int64_t(3));
}

TEST_CASE("Parsing index expression", "[parser]") {
  auto input = "myArray[1 + 1]";
  auto ast = parse("([parser]: Parsing index expression)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "CALL");

  testIdentifier(node->nodes[0], "myArray");

  REQUIRE(node->nodes[1]->name == "INDEX");
  testInfixExpression(node->nodes[1]->nodes[0], int64_t(1), "+", int64_t(1));
}

TEST_CASE("Parsing empty hash literal", "[parser]") {
  auto input = "{}";
  auto ast = parse("([parser]: Parsing empty hash literal)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "HASH");
}

TEST_CASE("Parsing hash literals string keys", "[parser]") {
  auto input = R"({"one": 1, "two": 2, "three": 3})";
  auto ast = parse("([parser]: Parsing hash literals string keys)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "HASH");

  map<string_view, int64_t> expected = {
      {"one"sv, 1},
      {"two"sv, 2},
      {"three"sv, 3},
  };

  for (auto node : node->nodes) {
    auto key = node->nodes[0];
    auto val = node->nodes[1];
    REQUIRE(key->name == "STRING");

    auto expectedValue = expected[key->token];
    testIntegerLiteral(val, expectedValue);
  }
}

TEST_CASE("Parsing hash literals boolean keys", "[parser]") {
  auto input = "{true: 1, false: 2}";
  auto ast = parse("([parser]: Parsing hash literals boolean keys)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "HASH");

  map<string_view, int64_t> expected = {
      {"true"sv, 1},
      {"false"sv, 2},
  };

  for (auto node : node->nodes) {
    auto key = node->nodes[0];
    auto val = node->nodes[1];
    REQUIRE(key->name == "BOOLEAN");

    auto expectedValue = expected[key->token];
    testIntegerLiteral(val, expectedValue);
  }
}

TEST_CASE("Parsing hash literals integer keys", "[parser]") {
  auto input = "{1: 1, 2: 2, 3: 3}";
  auto ast = parse("([parser]: Parsing hash literals integer keys)", input);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "HASH");

  map<string_view, int64_t> expected = {
      {"1"sv, 1},
      {"2"sv, 2},
      {"3"sv, 3},
  };

  for (auto node : node->nodes) {
    auto key = node->nodes[0];
    auto val = node->nodes[1];
    REQUIRE(key->name == "INTEGER");

    auto expectedValue = expected[key->token];
    testIntegerLiteral(val, expectedValue);
  }
}

TEST_CASE("Parsing hash literals with expression", "[parser]") {
  auto input = R"({"one": 0 + 1, "two": 10 - 8, "three": 15 / 5})";
  auto ast = parse("([parser]: Parsing hash literals with expression)", input);
  REQUIRE(ast != nullptr);
  REQUIRE(ast->name == "EXPRESSION_STATEMENT");

  auto node = ast->nodes[0];
  REQUIRE(node->name == "HASH");

  using TestFunc = function<void(const shared_ptr<Ast> &ast)>;
  map<string_view, TestFunc> tests = {
      {
          "one"sv,
          [](auto &ast) {
            testInfixExpression(ast, int64_t(0), "+", int64_t(1));
          },
      },
      {
          "two"sv,
          [](auto &ast) {
            testInfixExpression(ast, int64_t(10), "-", int64_t(8));
          },
      },
      {
          "three"sv,
          [](auto &ast) {
            testInfixExpression(ast, int64_t(15), "/", int64_t(5));
          },
      },
  };

  for (auto node : node->nodes) {
    auto key = node->nodes[0];
    auto val = node->nodes[1];
    REQUIRE(key->name == "STRING");

    auto testFunc = tests[key->token];
    testFunc(val);
  }
}
