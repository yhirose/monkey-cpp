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
    switch (constant->type()) {
    case INTEGER_OBJ:
      test_integer_object(cast<Integer>(constant).value, actual[i]);
      break;
    case STRING_OBJ:
      test_string_object(cast<String>(constant).value, actual[i]);
      break;
    default: break;
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
      {
          "-1",
          {make_integer(1)},
          {
              make(OpConstant, {0}),
              make(OpMinus, {}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Integer arithmetic)", tests);
}

TEST_CASE("Boolean expressions", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "true",
          {},
          {
              make(OpTrue, {}),
              make(OpPop, {}),
          },
      },
      {
          "false",
          {},
          {
              make(OpFalse, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 > 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpGreaterThan, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 < 2",
          {make_integer(2), make_integer(1)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpGreaterThan, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 == 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpEqual, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 != 2",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpNotEqual, {}),
              make(OpPop, {}),
          },
      },
      {
          "true == false",
          {},
          {
              make(OpTrue, {}),
              make(OpFalse, {}),
              make(OpEqual, {}),
              make(OpPop, {}),
          },
      },
      {
          "true != false",
          {},
          {
              make(OpTrue, {}),
              make(OpFalse, {}),
              make(OpNotEqual, {}),
              make(OpPop, {}),
          },
      },
      {
          "!true",
          {},
          {
              make(OpTrue, {}),
              make(OpBang, {}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Boolean expressions)", tests);
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

TEST_CASE("Conditionals", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "if (true) { 10 }; 3333;",
          {make_integer(10), make_integer(3333)},
          {
              /* 0000 */ make(OpTrue, {}),
              /* 0001 */ make(OpJumpNotTruthy, {10}),
              /* 0004 */ make(OpConstant, {0}),
              /* 0007 */ make(OpJump, {11}),
              /* 0010 */ make(OpNull, {}),
              /* 0011 */ make(OpPop, {}),
              /* 0012 */ make(OpConstant, {1}),
              /* 0015 */ make(OpPop, {}),
          },
      },
      {
          "if (true) { 10 } else { 20 } 3333;",
          {make_integer(10), make_integer(20), make_integer(3333)},
          {
              /* 0000 */ make(OpTrue, {}),
              /* 0001 */ make(OpJumpNotTruthy, {10}),
              /* 0004 */ make(OpConstant, {0}),
              /* 0007 */ make(OpJump, {13}),
              /* 0010 */ make(OpConstant, {1}),
              /* 0013 */ make(OpPop, {}),
              /* 0014 */ make(OpConstant, {2}),
              /* 0017 */ make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Conditionals)", tests);
}

TEST_CASE("Global Let Statements", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          R"(
            let one = 1;
            let two = 2;
          )",
          {make_integer(1), make_integer(2)},
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpConstant, {1}),
              make(OpSetGlobal, {1}),
          },
      },
      {
          R"(
            let one = 1;
            one;
          )",
          {make_integer(1)},
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpGetGlobal, {0}),
              make(OpPop, {}),
          },
      },
      {
          R"(
            let one = 1;
            let two = one;
            two;
          )",
          {make_integer(1)},
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpGetGlobal, {0}),
              make(OpSetGlobal, {1}),
              make(OpGetGlobal, {1}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Conditionals)", tests);
}

TEST_CASE("String expressions", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          R"("monkey")",
          {make_string("monkey")},
          {
              make(OpConstant, {0}),
              make(OpPop, {}),
          },
      },
      {
          R"("mon" + "key")",
          {make_string("mon"), make_string("key")},
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpAdd, {}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: String expressions)", tests);
}
