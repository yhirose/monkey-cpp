#include <evaluator.hpp>

#include "catch.hh"
#include "test-util.hpp"

using namespace std;
using namespace peg::udl;
using namespace monkey;

shared_ptr<Object> testEval(const string &input) {
  auto ast = parse("([evaluator])", input);
  // cout << ast_to_s(ast) << endl;
  REQUIRE(ast != nullptr);
  return eval(ast, monkey::environment());
}

void testIntegerObject(shared_ptr<Object> evaluated, int64_t expected) {
  REQUIRE(evaluated->type() == INTEGER_OBJ);
  CHECK(cast<Integer>(evaluated).value == expected);
}

void testBooleanObject(shared_ptr<Object> evaluated, int64_t expected) {
  REQUIRE(evaluated->type() == BOOLEAN_OBJ);
  CHECK(cast<Boolean>(evaluated).value == expected);
}

void testNullObject(shared_ptr<Object> evaluated) {
  REQUIRE(evaluated->type() == NULL_OBJ);
  CHECK(evaluated.get() == CONST_NULL.get());
}

void testStringObject(shared_ptr<Object> evaluated, const char *expected) {
  CHECK(evaluated->type() == STRING_OBJ);
  CHECK(cast<String>(evaluated).value == expected);
}

void testObject(shared_ptr<Object> evaluated,
                const shared_ptr<Object> expected) {
  CHECK(evaluated->type() == expected->type());
  if (evaluated->type() == INTEGER_OBJ) {
    CHECK(cast<Integer>(evaluated).value == cast<Integer>(expected).value);
  } else if (evaluated->type() == ERROR_OBJ) {
    CHECK(cast<Error>(evaluated).message == cast<Error>(expected).message);
  } else if (evaluated->type() == NULL_OBJ) {
    testNullObject(expected);
    testNullObject(evaluated);
  }
}

TEST_CASE("Eval integer expression", "[evaluator]") {
  struct Test {
    string input;
    int64_t expected;
  };

  Test tests[] = {
      {"5", 5},
      {"10", 10},
      {"-5", -5},
      {"-10", -10},
      {"5 + 5 + 5 + 5 - 10", 10},
      {"2 * 2 * 2 * 2 * 2", 32},
      {"-50 + 100 + -50", 0},
      {"5 * 2 + 10", 20},
      {"5 + 2 * 10", 25},
      {"20 + 2 * -10", 0},
      {"50 / 2 * 2 + 10", 60},
      {"2 * (5 + 10)", 30},
      {"3 * 3 * 3 + 10", 37},
      {"3 * (3 * 3) + 10", 37},
      {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
  };

  for (const auto &t : tests) {
    testIntegerObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Eval boolean expression", "[evaluator]") {
  struct Test {
    string input;
    bool expected;
  };

  Test tests[] = {
      {"true", true},
      {"false", false},
      {"1 < 2", true},
      {"1 > 2", false},
      {"1 < 1", false},
      {"1 > 1", false},
      {"1 == 1", true},
      {"1 != 1", false},
      {"1 == 2", false},
      {"1 != 2", true},
      {"true == true", true},
      {"false == false", true},
      {"true == false", false},
      {"true != false", true},
      {"false != true", true},
      {"(1 < 2) == true", true},
      {"(1 < 2) == false", false},
      {"(1 > 2) == true", false},
      {"(1 > 2) == false", true},
  };

  for (const auto &t : tests) {
    testBooleanObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Bang operator", "[evaluator]") {
  struct Test {
    string input;
    bool expected;
  };

  Test tests[] = {
      {"!true", false}, {"!false", true},   {"!5", false},
      {"!!true", true}, {"!!false", false}, {"!!5", true},
  };

  for (const auto &t : tests) {
    testBooleanObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("If else expressions", "[evaluator]") {
  struct Test {
    string input;
    shared_ptr<Object> expected;
  };

  Test tests[] = {
      {"if (true) { 10 }", make_integer(10)},
      {"if (false) { 10 }", CONST_NULL},
      {"if (1) { 10 }", make_integer(10)},
      {"if (1 < 2) { 10 }", make_integer(10)},
      {"if (1 > 2) { 10 }", CONST_NULL},
      {"if (1 > 2) { 10 } else { 20 }", make_integer(20)},
      {"if (1 < 2) { 10 } else { 20 }", make_integer(10)},
  };

  for (const auto &t : tests) {
    testObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Return statements", "[evaluator]") {
  struct Test {
    string input;
    int64_t expected;
  };

  Test tests[] = {
      {"return 10;", 10},
      {"return 10; 9;", 10},
      {"return 2 * 5; 9;", 10},
      {"9; return 2 * 5; 9;", 10},
      {"if (10 > 1) { return 10; }", 10},
      {
          R"(
if (10 > 1) {
  if (10 > 1) {
    return 10;
  }

  return 1;
}
)",
          10,
      },
      {
          R"(
let f = fn(x) {
  return x;
  x + 10;
};
f(10);)",
          10,
      },
      {
          R"(
let f = fn(x) {
   let result = x + 10;
   return result;
   return 10;
};
f(10);)",
          20,
      },
  };

  for (const auto &t : tests) {
    testIntegerObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Error handling", "[evaluator]") {
  struct Test {
    string input;
    string expectedMessage;
  };

  Test tests[] = {
      {
          "5 + true;",
          "type mismatch: INTEGER + BOOLEAN",
      },
      {
          "5 + true; 5;",
          "type mismatch: INTEGER + BOOLEAN",
      },
      {
          "-true",
          "unknown operator: -BOOLEAN",
      },
      {
          "-true",
          "unknown operator: -BOOLEAN",
      },
      {
          "true + false;",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "true + false + true + false;",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "5; true + false; 5",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "if (10 > 1) { true + false; }",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          R"(
if (10 > 1) {
  if (10 > 1) {
    return true + false;
  }

  return 1;
}
      )",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "foobar",
          "identifier not found: foobar",
      },
      {
          R"("Hello" - "World")",
          "unknown operator: STRING - STRING",
      },
      {
          R"({"name": "Monkey"}[fn(x) { x }];)",
          "unusable as hash key: FUNCTION",
      },
  };

  for (const auto &t : tests) {
    auto ast = parse("([evaluator])", t.input);
    REQUIRE(ast != nullptr);

    auto env = monkey::environment();
    auto val = eval(ast, env);
    CHECK(val->type() == ERROR_OBJ);

    CHECK(cast<Error>(val).message == t.expectedMessage);
  }
}

TEST_CASE("Let statements", "[evaluator]") {
  struct Test {
    string input;
    int64_t expected;
  };

  Test tests[] = {
      {"let a = 5; a;", 5},
      {"let a = 5 * 5; a;", 25},
      {"let a = 5; let b = a; b;", 5},
      {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
  };

  for (const auto &t : tests) {
    testIntegerObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Function object", "[evaluator]") {
  auto input = "fn(x) { x + 2; };";

  auto val = testEval(input);
  REQUIRE(val->type() == FUNCTION_OBJ);

  const auto &fn = cast<Function>(val);

  REQUIRE(fn.params.size() == 1);
  CHECK(fn.params[0] == "x");

  const auto expectedBody = "(x + 2)";
  CHECK(to_string(fn.body) == expectedBody);
}

TEST_CASE("Function application", "[evaluator]") {
  struct Test {
    string input;
    int64_t expected;
  };

  Test tests[] = {
      {"let identity = fn(x) { x; }; identity(5);", 5},
      {"let identity = fn(x) { return x; }; identity(5);", 5},
      {"let double = fn(x) { x * 2; }; double(5);", 10},
      {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
      {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
      {"fn(x) { x; }(5)", 5},
  };

  for (const auto &t : tests) {
    testIntegerObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Enclosing environments", "[evaluator]") {
  auto input = R"(
let first = 10;
let second = 10;
let third = 10;

let ourFunction = fn(first) {
  let second = 20;

  first + second + third;
};

ourFunction(20) + first + second;
  )";

  testIntegerObject(testEval(input), 70);
}

TEST_CASE("Closures", "[evaluator]") {
  auto input = R"(
let newAdder = fn(x) {
  fn(y) { x + y };
};

let addTwo = newAdder(2);
addTwo(2);
  )";

  testIntegerObject(testEval(input), 4);
}

TEST_CASE("String literal", "[evaluator]") {
  auto input = R"("Hello World!")";
  testStringObject(testEval(input), "Hello World!");
}

TEST_CASE("Test string concatenation", "[evaluator]") {
  auto input = R"("Hello" + " " + "World!")";
  testStringObject(testEval(input), "Hello World!");
}

TEST_CASE("Test builtin functions", "[evaluator]") {
  struct Test {
    string input;
    shared_ptr<Object> expected;
  };

  Test tests[] = {
      {R"(len(""))", make_integer(0)},
      {R"(len("four"))", make_integer(4)},
      {R"(len("hello world"))", make_integer(11)},
      {R"(len(1))", make_error("argument to `len` not supported, got INTEGER")},
      {R"(len("one", "two"))",
       make_error("wrong number of arguments. got=2, want=1")},
      {R"(len([1, 2, 3]))", make_integer(3)},
      {R"(len([]))", make_integer(0)},
      {R"(puts("hello", "world!"))", CONST_NULL},
      {R"(first([1, 2, 3]))", make_integer(1)},
      {R"(first([]))", CONST_NULL},
      {R"(first(1))",
       make_error("argument to `first` must be ARRAY, got INTEGER")},
      {R"(last([1, 2, 3]))", make_integer(3)},
      {R"(last([]))", CONST_NULL},
      {R"(last(1))",
       make_error("argument to `last` must be ARRAY, got INTEGER")},
      {R"(rest([1, 2, 3]))", make_array({2, 3})},
      {R"(rest([]))", CONST_NULL},
      {R"(push([], 1))", make_array({1})},
      {R"(push(1, 1))",
       make_error("argument to `push` must be ARRAY, got INTEGER")},
  };

  for (const auto &t : tests) {
    testObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Test array literals", "[evaluator]") {
  auto input = "[1, 2 * 2, 3 + 3]";
  auto val = testEval(input);
  REQUIRE(val->type() == ARRAY_OBJ);

  const auto &arr = cast<Array>(val);
  REQUIRE(arr.elements.size() == 3);

  testIntegerObject(arr.elements[0], 1);
  testIntegerObject(arr.elements[1], 4);
  testIntegerObject(arr.elements[2], 6);
}

TEST_CASE("Test array index expressions", "[evaluator]") {
  struct Test {
    string input;
    shared_ptr<Object> expected;
  };

  Test tests[] = {
      {"[1, 2, 3][0]", make_integer(1)},
      {"[1, 2, 3][1]", make_integer(2)},
      {"[1, 2, 3][2]", make_integer(3)},
      {"let i = 0; [1][i];", make_integer(1)},
      {"[1, 2, 3][1 + 1];", make_integer(3)},
      {"let myArray = [1, 2, 3]; myArray[2];", make_integer(3)},
      {"let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];",
       make_integer(6)},
      {"let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]",
       make_integer(2)},
      {"[1, 2, 3][3]", CONST_NULL},
      {"[1, 2, 3][-1]", CONST_NULL},
  };

  for (const auto &t : tests) {
    testObject(testEval(t.input), t.expected);
  }
}

TEST_CASE("Test hash literals", "[evaluator]") {
  auto input = R"(let two = "two";
	{
		"one": 10 - 9,
		two: 1 + 1,
		"thr" + "ee": 6 / 2,
		4: 4,
		true: 5,
		false: 6
	})";

  auto evaluated = testEval(input);
  REQUIRE(evaluated->type() == HASH_OBJ);

  map<HashKey, int64_t> expected{
      {make_string("one")->hash_key(), int64_t(1)},
      {make_string("two")->hash_key(), int64_t(2)},
      {make_string("three")->hash_key(), int64_t(3)},
      {make_integer(4)->hash_key(), int64_t(4)},
      {CONST_TRUE->hash_key(), int64_t(5)},
      {CONST_FALSE->hash_key(), int64_t(6)},
  };

  REQUIRE(cast<Hash>(evaluated).pairs.size() == expected.size());

  for (auto [expectedKey, expectedValue] : expected) {
    auto &pair = cast<Hash>(evaluated).pairs[expectedKey];
    testIntegerObject(pair.value, expectedValue);
  }
}

TEST_CASE("Test hash index expressions", "[evaluator]") {
  struct Test {
    string input;
    shared_ptr<Object> expected;
  };

  Test tests[] = {
      {R"({"foo": 5}["foo"])", make_integer(5)},
      {R"({"foo": 5}["bar"])", CONST_NULL},
      {R"(let key = "foo"; {"foo": 5}[key])", make_integer(5)},
      {R"({}["foo"])", CONST_NULL},
      {R"({5: 5}[5])", make_integer(5)},
      {R"({true: 5}[true])", make_integer(5)},
      {R"({false: 5}[false])", make_integer(5)},
  };

  for (const auto &t : tests) {
    testObject(testEval(t.input), t.expected);
  }
}
