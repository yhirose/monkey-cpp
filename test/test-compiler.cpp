#include "catch.hh"
#include "test-util.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <compiler.hpp>

using namespace std;
using namespace monkey;

void test_instructions(vector<Instructions> expected, Instructions actual) {
  auto concatted = concat_instructions(expected);
  REQUIRE(actual.size() == concatted.size());

  size_t i = 0;
  for (const auto &ins : concatted) {
    CHECK(actual[i] == ins);
    i++;
  }
}

void test_constants(vector<shared_ptr<Object>> expected,
                    vector<shared_ptr<Object>> actual) {
  REQUIRE(expected.size() == actual.size());

  size_t i = 0;
  for (const auto &constant : expected) {
    if (constant->type() == INTEGER_OBJ) {
      auto val = cast<Integer>(constant).value;
      test_integer_object(val, actual[i]);
    }
    i++;
  }
}

struct CompilerTestCase {
  string input;
  vector<shared_ptr<Object>> expectedConstants;
  vector<Instructions> expectedInstructions;
};

void run_compiler_test(const char *name,
                       const vector<CompilerTestCase> &tests) {
  for (const auto &t : tests) {
    auto ast = parse(name, t.input);
    // cerr << peg::ast_to_s(ast) << endl;
    REQUIRE(ast != nullptr);

    Compiler compiler;
    compiler.compile(ast);
    auto bytecode = compiler.bytecode();

    test_instructions(t.expectedInstructions, bytecode.instructions);
    test_constants(t.expectedConstants, bytecode.constants);
  }
}

TEST_CASE("Integer arithmetic", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "1 + 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpAdd, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 - 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpSub, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 * 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpMul, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 / 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpDiv, {}),
              make(OpPop, {}),
          },
      },
      {
          "1; 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpPop, {}),
              make(OpConstant, {1}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Integer arithmetic)", tests);
}

TEST_CASE("Read operands", "[compiler]") {
  struct Test {
    OpecodeType op;
    vector<int> operands;
    size_t byteRead;
  };

  Test tests[] = {
      {OpConstant, {65535}, 2},
  };

  for (const auto &t : tests) {
    auto instruction = make(t.op, t.operands);
    auto &def = lookup(t.op);

    auto [operandsRead, n] = read_operands(def, instruction, 1);
    CHECK(n == t.byteRead);

    size_t i = 0;
    for (auto want : t.operands) {
      CHECK(operandsRead[i] == want);
      i++;
    }
  }
}

