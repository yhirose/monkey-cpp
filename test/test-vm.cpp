#include "catch.hh"
#include "test-util.hpp"

#include <compiler.hpp>
#include <vm.hpp>

using namespace std;
using namespace monkey;

void test_expected_object(shared_ptr<Object> expected,
                          shared_ptr<Object> actual) {
  switch (expected->type()) {
  case INTEGER_OBJ:
    test_integer_object(cast<Integer>(expected).value, actual);
    break;
  case BOOLEAN_OBJ:
    test_boolean_object(cast<Boolean>(expected).value, actual);
    break;
  case STRING_OBJ:
    test_string_object(cast<String>(expected).value, actual);
    break;
  case NULL_OBJ: test_null_object(actual); break;
  case ARRAY_OBJ: {
    REQUIRE(actual);
    const auto &expectedElements = cast<Array>(expected).elements;
    const auto &actualElements = cast<Array>(actual).elements;
    REQUIRE(expectedElements.size() == actualElements.size());
    for (size_t i = 0; i < expectedElements.size(); i++) {
      REQUIRE(expectedElements[i] != actualElements[i]);
    }
    break;
  }
  default: break;
  }
}

struct VmTestCase {
  string input;
  shared_ptr<Object> expected;
};

void run_vm_test(const char *name, const vector<VmTestCase> &tests) {
  for (const auto &t : tests) {
    auto ast = parse(name, t.input);
    // cerr << peg::ast_to_s(ast) << endl;
    REQUIRE(ast != nullptr);

    Compiler compiler;
    compiler.compile(ast);

    VM vm(compiler.bytecode());
    vm.run();

    auto stack_elem = vm.last_popped_stack_elem();

    test_expected_object(t.expected, stack_elem);
  }
}

TEST_CASE("Integer arithmetic - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"1", make_integer(1)},
      {"2", make_integer(2)},
      {"1 + 2", make_integer(3)},
      {"1 - 2", make_integer(-1)},
      {"1 * 2", make_integer(2)},
      {"4 / 2", make_integer(2)},
      {"50 / 2 * 2 + 10 - 5", make_integer(55)},
      {"5 + 5 + 5 + 5 - 10", make_integer(10)},
      {"2 * 2 * 2 * 2 * 2", make_integer(32)},
      {"5 * 2 + 10", make_integer(20)},
      {"5 + 2 * 10", make_integer(25)},
      {"5 * (2 + 10)", make_integer(60)},
      {"-5", make_integer(-5)},
      {"-10", make_integer(-10)},
      {"-50 + 100 + -50", make_integer(0)},
      {"(5 + 10 * 2 + 15 / 3) * 2 + -10", make_integer(50)},
  };

  run_vm_test("([vm]: Integer arithmetic)", tests);
}

TEST_CASE("Boolean expressions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"true", make_bool(true)},
      {"false", make_bool(false)},
      {"1 < 2", make_bool(true)},
      {"1 > 2", make_bool(false)},
      {"1 < 1", make_bool(false)},
      {"1 > 1", make_bool(false)},
      {"1 == 1", make_bool(true)},
      {"1 != 1", make_bool(false)},
      {"1 == 2", make_bool(false)},
      {"1 != 2", make_bool(true)},
      {"true == true", make_bool(true)},
      {"false == false", make_bool(true)},
      {"true == false", make_bool(false)},
      {"true != false", make_bool(true)},
      {"false != true", make_bool(true)},
      {"(1 < 2) == true", make_bool(true)},
      {"(1 < 2) == false", make_bool(false)},
      {"(1 > 2) == true", make_bool(false)},
      {"(1 > 2) == false", make_bool(true)},
      {"!true", make_bool(false)},
      {"!false", make_bool(true)},
      {"!5", make_bool(false)},
      {"!!true", make_bool(true)},
      {"!!false", make_bool(false)},
      {"!!5", make_bool(true)},
      {"!(if (false) { 5; })", make_bool(true)},
  };

  run_vm_test("([vm]: Boolean expressions)", tests);
}

TEST_CASE("Conditionals - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"if (true) { 10 }", make_integer(10)},
      {"if (true) { 10 } else { 20 }", make_integer(10)},
      {"if (false) { 10 } else { 20 }", make_integer(20)},
      {"if (1) { 10 }", make_integer(10)},
      {"if (1 < 2) { 10 }", make_integer(10)},
      {"if (1 < 2) { 10 } else { 20 }", make_integer(10)},
      {"if (1 > 2) { 10 } else { 20 }", make_integer(20)},
      {"if (1 > 2) { 10 }", CONST_NULL},
      {"if (false) { 10 }", CONST_NULL},
      {"if ((if (false) { 10 })) { 10 } else { 20 }", make_integer(20)},
  };

  run_vm_test("([vm]: Conditionals)", tests);
}

TEST_CASE("Global Let Statements - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"let one = 1; one", make_integer(1)},
      {"let one = 1; let two = 2; one + two", make_integer(3)},
      {"let one = 1; let two = one + one; one + two", make_integer(3)},
  };

  run_vm_test("([vm]: Global Let Statements)", tests);
}

TEST_CASE("String expressions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"("monkey")", make_string("monkey")},
      {R"("mon" + "key")", make_string("monkey")},
      {R"("mon" + "key" + "banana")", make_string("monkeybanana")},
  };

  run_vm_test("([vm]: String expressions)", tests);
}

TEST_CASE("Arrray Literals - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"[]", make_array({})},
      {"[1, 2, 3]", make_array({1, 2, 3})},
      {"[1 + 2, 3 * 4, 5 + 6]", make_array({3, 12, 11})},
  };

  run_vm_test("([vm]: Arrray Literals)", tests);
}

TEST_CASE("Hash Literals - vm", "[vm]") {
  struct VmHashTestCase {
    string input;
    map<HashKey, std::shared_ptr<Object>> expected;
  };

  vector<VmHashTestCase> tests{
      {"{}", {}},
      {"{1: 2, 2: 3}",
       {
           {make_integer(1)->hash_key(), make_integer(2)},
           {make_integer(2)->hash_key(), make_integer(3)},
       }},
      {"{1 + 1: 2 * 2, 3 + 3: 4 * 4}",
       {
           {make_integer(2)->hash_key(), make_integer(4)},
           {make_integer(6)->hash_key(), make_integer(16)},
       }},
  };

  for (const auto &t : tests) {
    auto ast = parse("([vm]: Arrray Literals)", t.input);
    REQUIRE(ast != nullptr);

    Compiler compiler;
    compiler.compile(ast);

    VM vm(compiler.bytecode());
    vm.run();

    auto actual = vm.last_popped_stack_elem();
    REQUIRE(actual);

    auto &actualParis = cast<Hash>(actual).pairs;
    REQUIRE(t.expected.size() == actualParis.size());

    for (auto &[expectedKey, expectedValue] : t.expected) {
      auto pair = actualParis[expectedKey];
      test_integer_object(cast<Integer>(expectedValue).value, pair.value);
    }
  }
}

TEST_CASE("Index Expressions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {"[1, 2, 3][1]", make_integer(2)},
      {"[1, 2, 3][0 + 2]", make_integer(3)},
      {"[[1, 1, 1]][0][0]", make_integer(1)},
      {"[][0]", CONST_NULL},
      {"[1, 2, 3][99]", CONST_NULL},
      {"[1][-1]", CONST_NULL},
      {"{1: 1, 2: 2}[1]", make_integer(1)},
      {"{1: 1, 2: 2}[2]", make_integer(2)},
      {"{1: 1}[0]", CONST_NULL},
      {"{}[0]", CONST_NULL},
  };

  run_vm_test("([vm]: Index Expressions)", tests);
}

TEST_CASE("Calling Functions Without Arguments - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"(
         let fivePlusTen = fn() { 5 + 10; };
         fivePlusTen();
       )",
       make_integer(15)},
      {R"(
         let one = fn() { 1; };
         let two = fn() { 2; };
         one() + two();
       )",
       make_integer(3)},
      {R"(
         let a = fn() { 1; };
         let b = fn() { a() + 1; };
         let c = fn() { b() + 1; };
         c();
       )",
       make_integer(3)},
  };

  run_vm_test("([vm]: Calling Functions Without Arguments)", tests);
}

TEST_CASE("Functions With Return Statement - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"(
         let earlyExit = fn() { return 99; 100; };
         earlyExit();
       )",
       make_integer(99)},
      {R"(
         let earlyExit = fn() { return 99; return 100; };
         earlyExit();
       )",
       make_integer(99)},
  };

  run_vm_test("([vm]: Functions With Return Statement)", tests);
}

TEST_CASE("Functions Without Return Statement - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"(
         let noReturn = fn() { };
         noReturn();
       )",
       CONST_NULL},
      {R"(
         let noReturn = fn() { };
         let noReturnTwo = fn() { noReturn(); };
         noReturn();
         noReturnTwo();
       )",
       CONST_NULL},
  };

  run_vm_test("([vm]: Functions Without Return Statement)", tests);
}

TEST_CASE("First Class Functions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"(
         let returnOne = fn() { 1; };
         let returnsOneReturner = fn() { returnOne; };
         returnsOneReturner()();
       )",
       make_integer(1)},
      {R"(
         let returnsOneReturner = fn() {
           let retunrsOne = fn() { 1; };
           retunrsOne;
         };
         returnsOneReturner()();
       )",
       make_integer(1)},
  };

  run_vm_test("([vm]: First Class Functions)", tests);
}

TEST_CASE("Calling Functions With Bindings - vm", "[vm]") {
  vector<VmTestCase> tests{
      {R"(
         let one = fn() { let one = 1; one };
         one();
       )",
       make_integer(1)},
      {R"(
         let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };
         oneAndTwo();
       )",
       make_integer(3)},
      {R"(
         let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };
         let threeAndFour = fn() { let three = 3; let four = 4; three + four; };
         oneAndTwo() + threeAndFour();
       )",
       make_integer(10)},
      {R"(
         let firstFoobar = fn() { let foobar = 50; foobar; };
         let secondFoobar = fn() { let foobar = 100; foobar; };
         firstFoobar() + secondFoobar();
       )",
       make_integer(150)},
      {R"(
         let globalSeed = 50;
         let minusOne = fn() {
           let num = 1;
           globalSeed - num;
         }
         let minusTwo = fn() {
           let num = 2;
           globalSeed - num;
         }
         minusOne() + minusTwo();
       )",
       make_integer(97)},
  };

  run_vm_test("([vm]: Calling Functions With Bindings)", tests);
}
