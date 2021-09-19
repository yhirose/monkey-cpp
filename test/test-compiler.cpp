#include "catch.hh"
#include "test-util.hpp"
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <compiler.hpp>

using namespace std;
using namespace monkey;

void test_instructions(const Instructions &expected,
                       const Instructions &actual) {
  REQUIRE(to_string(expected) == to_string(actual));

  size_t i = 0;
  for (const auto &ins : expected) {
    CHECK(actual[i] == ins);
    i++;
  }
}

void test_constants(const vector<shared_ptr<Object>> &expected,
                    const vector<shared_ptr<Object>> &actual) {
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
    case COMPILED_FUNCTION_OBJ: {
      test_instructions(cast<CompiledFunction>(constant).instructions,
                        cast<CompiledFunction>(actual[i]).instructions);
      break;
    }
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

    test_instructions(concat_instructions(t.expectedInstructions),
                      bytecode.instructions);
    test_constants(t.expectedConstants, bytecode.constants);
  }
}

TEST_CASE("Integer arithmetic", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "1 + 2",
          {
              make_integer(1),
              make_integer(2),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpAdd, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 - 2",
          {
              make_integer(1),
              make_integer(2),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpSub, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 * 2",
          {
              make_integer(1),
              make_integer(2),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpMul, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 / 2",
          {
              make_integer(1),
              make_integer(2),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpDiv, {}),
              make(OpPop, {}),
          },
      },
      {
          "1; 2",
          {
              make_integer(1),
              make_integer(2),
          },
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
          {
              make_integer(1),
              make_integer(2),
          },
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
          {
              make_integer(1),
              make_integer(2),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpEqual, {}),
              make(OpPop, {}),
          },
      },
      {
          "1 != 2",
          {
              make_integer(1),
              make_integer(2),
          },
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
          {
              make_integer(10),
              make_integer(3333),
          },
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
          {
              make_integer(10),
              make_integer(20),
              make_integer(3333),
          },
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
          {
              make_integer(1),
              make_integer(2),
          },
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
          {
              make_integer(1),
          },
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

  run_compiler_test("([compiler]: Global Let Statements)", tests);
}

TEST_CASE("String expressions", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          R"("monkey")",
          {
              make_string("monkey"),
          },
          {
              make(OpConstant, {0}),
              make(OpPop, {}),
          },
      },
      {
          R"("mon" + "key")",
          {
              make_string("mon"),
              make_string("key"),
          },
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

TEST_CASE("Array Literals", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "[]",
          {},
          {
              make(OpArray, {0}),
              make(OpPop, {}),
          },
      },
      {
          "[1, 2, 3]",
          {
              make_integer(1),
              make_integer(2),
              make_integer(3),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpConstant, {2}),
              make(OpArray, {3}),
              make(OpPop, {}),
          },
      },
      {
          "[1 + 2, 3 - 4, 5 * 6]",
          {
              make_integer(1),
              make_integer(2),
              make_integer(3),
              make_integer(4),
              make_integer(5),
              make_integer(6),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpAdd, {}),
              make(OpConstant, {2}),
              make(OpConstant, {3}),
              make(OpSub, {}),
              make(OpConstant, {4}),
              make(OpConstant, {5}),
              make(OpMul, {}),
              make(OpArray, {3}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Array Literals)", tests);
}

TEST_CASE("Hash Literals", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "{}",
          {},
          {
              make(OpHash, {0}),
              make(OpPop, {}),
          },
      },
      {
          "{1: 2, 3: 4, 5: 6}",
          {
              make_integer(1),
              make_integer(2),
              make_integer(3),
              make_integer(4),
              make_integer(5),
              make_integer(6),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpConstant, {2}),
              make(OpConstant, {3}),
              make(OpConstant, {4}),
              make(OpConstant, {5}),
              make(OpHash, {6}),
              make(OpPop, {}),
          },
      },
      {
          "{1: 2 + 3, 4: 5 * 6}",
          {
              make_integer(1),
              make_integer(2),
              make_integer(3),
              make_integer(4),
              make_integer(5),
              make_integer(6),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpConstant, {2}),
              make(OpAdd, {}),
              make(OpConstant, {3}),
              make(OpConstant, {4}),
              make(OpConstant, {5}),
              make(OpMul, {}),
              make(OpHash, {4}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Hash Literals)", tests);
}

TEST_CASE("Index Expressions", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "[1, 2, 3][1 + 1]",
          {
              make_integer(1),
              make_integer(2),
              make_integer(3),
              make_integer(1),
              make_integer(1),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpConstant, {2}),
              make(OpArray, {3}),
              make(OpConstant, {3}),
              make(OpConstant, {4}),
              make(OpAdd, {}),
              make(OpIndex, {}),
              make(OpPop, {}),
          },
      },
      {
          "{1: 2}[2 - 1]",
          {
              make_integer(1),
              make_integer(2),
              make_integer(2),
              make_integer(1),
          },
          {
              make(OpConstant, {0}),
              make(OpConstant, {1}),
              make(OpHash, {2}),
              make(OpConstant, {2}),
              make(OpConstant, {3}),
              make(OpSub, {}),
              make(OpIndex, {}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Index Expressions)", tests);
}

TEST_CASE("Functions", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "fn() { return 5 + 10 }",
          {
              make_integer(5),
              make_integer(10),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpConstant, {1}),
                  make(OpAdd, {}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {2}),
              make(OpPop, {}),
          },
      },
      {
          "fn() { 5 + 10 }",
          {
              make_integer(5),
              make_integer(10),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpConstant, {1}),
                  make(OpAdd, {}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {2}),
              make(OpPop, {}),
          },
      },
      {
          "fn() { 1; 2 }",
          {
              make_integer(1),
              make_integer(2),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpPop, {}),
                  make(OpConstant, {1}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {2}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Functions)", tests);
}

TEST_CASE("Functions Without Return Value", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "fn() { }",
          {
              make_compiled_function({
                  make(OpReturn, {}),
              }),
          },
          {
              make(OpConstant, {0}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Functions Without Return Value)", tests);
}

TEST_CASE("Function Calls", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          "fn() { 24 }();",
          {
              make_integer(24),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {1}),
              make(OpCall, {0}),
              make(OpPop, {}),
          },
      },
      {
          R"(
          let noArg = fn() { 24 };
          noArg();
          )",
          {
              make_integer(24),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {1}),
              make(OpSetGlobal, {0}),
              make(OpGetGlobal, {0}),
              make(OpCall, {0}),
              make(OpPop, {}),
          },
      },
      {
          R"(
          let oneArg = fn(a) { a };
          oneArg(24);
          )",
          {
              make_compiled_function({
                  make(OpGetLocal, {0}),
                  make(OpReturnValue, {}),
              }),
              make_integer(24),
          },
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpGetGlobal, {0}),
              make(OpConstant, {1}),
              make(OpCall, {1}),
              make(OpPop, {}),
          },
      },
      {
          R"(
          let manyArg = fn(a, b, c) { a; b; c };
          manyArg(24, 25, 26);
          )",
          {
              make_compiled_function({
                  make(OpGetLocal, {0}),
                  make(OpPop, {}),
                  make(OpGetLocal, {1}),
                  make(OpPop, {}),
                  make(OpGetLocal, {2}),
                  make(OpReturnValue, {}),
              }),
              make_integer(24),
              make_integer(25),
              make_integer(26),
          },
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpGetGlobal, {0}),
              make(OpConstant, {1}),
              make(OpConstant, {2}),
              make(OpConstant, {3}),
              make(OpCall, {3}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Function Calls)", tests);
}

TEST_CASE("Let Statement Scopes", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          R"(
            let num = 55;
            fn() { num }
          )",
          {
              make_integer(55),
              make_compiled_function({
                  make(OpGetGlobal, {0}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {0}),
              make(OpSetGlobal, {0}),
              make(OpConstant, {1}),
              make(OpPop, {}),
          },
      },
      {
          R"(
            fn() {
              let num = 55;
              num
            }
          )",
          {
              make_integer(55),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpSetLocal, {0}),
                  make(OpGetLocal, {0}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {1}),
              make(OpPop, {}),
          },
      },
      {
          R"(
            fn() {
              let a = 55;
              let b = 77;
              a + b
            }
          )",
          {
              make_integer(55),
              make_integer(77),
              make_compiled_function({
                  make(OpConstant, {0}),
                  make(OpSetLocal, {0}),
                  make(OpConstant, {1}),
                  make(OpSetLocal, {1}),
                  make(OpGetLocal, {0}),
                  make(OpGetLocal, {1}),
                  make(OpAdd, {}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {2}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Let Statement Scopes)", tests);
}

TEST_CASE("Compiler Scopes", "[compiler]") {
  Compiler compiler;
  REQUIRE(compiler.scopeIndex == 0);

  auto globalSymbolTable = compiler.symbolTable;

  compiler.emit(OpMul, {});

  compiler.enter_scope();
  REQUIRE(compiler.scopeIndex == 1);

  compiler.emit(OpSub, {});

  REQUIRE(compiler.current_instructions().size() == 1);

  auto last = compiler.last_instruction();
  REQUIRE(last.opecode == OpSub);

  REQUIRE(compiler.symbolTable->outer == globalSymbolTable);

  compiler.leave_scope();
  REQUIRE(compiler.scopeIndex == 0);

  REQUIRE(compiler.symbolTable == globalSymbolTable);
  REQUIRE(!compiler.symbolTable->outer);

  compiler.emit(OpAdd, {});
  REQUIRE(compiler.current_instructions().size() == 2);

  last = compiler.last_instruction();
  REQUIRE(last.opecode == OpAdd);

  auto previous = compiler.previous_instruction();
  REQUIRE(previous.opecode == OpMul);
}

TEST_CASE("Builtins", "[compiler]") {
  vector<CompilerTestCase> tests{
      {
          R"(
            len([]);
            push([], 1);
          )",
          {
              make_integer(1),
          },
          {
              make(OpGetBuiltin, {0}),
              make(OpArray, {0}),
              make(OpCall, {1}),
              make(OpPop, {}),
              make(OpGetBuiltin, {5}),
              make(OpArray, {0}),
              make(OpConstant, {0}),
              make(OpCall, {2}),
              make(OpPop, {}),
          },
      },
      {
          R"(
            fn() { len([]); }
          )",
          {
              make_compiled_function({
                  make(OpGetBuiltin, {0}),
                  make(OpArray, {0}),
                  make(OpCall, {1}),
                  make(OpReturnValue, {}),
              }),
          },
          {
              make(OpConstant, {0}),
              make(OpPop, {}),
          },
      },
  };

  run_compiler_test("([compiler]: Builtins)", tests);
}
