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
      {
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
      },
  };

  run_vm_test("([vm]: Integer arithmetic)", tests);
}

TEST_CASE("Boolean expressions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {
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
      },
  };

  run_vm_test("([vm]: Boolean expressions)", tests);
}

TEST_CASE("Conditionals - vm", "[vm]") {
  vector<VmTestCase> tests{
      {
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
      },
  };

  run_vm_test("([vm]: Conditionals)", tests);
}

TEST_CASE("Global Let Statements - vm", "[vm]") {
  vector<VmTestCase> tests{
      {
          {"let one = 1; one", make_integer(1)},
          {"let one = 1; let two = 2; one + two", make_integer(3)},
          {"let one = 1; let two = one + one; one + two", make_integer(3)},
      },
  };

  run_vm_test("([vm]: Global Let Statements)", tests);
}

TEST_CASE("String expressions - vm", "[vm]") {
  vector<VmTestCase> tests{
      {
          {R"("monkey")", make_string("monkey")},
          {R"("mon" + "key")", make_string("monkey")},
          {R"("mon" + "key" + "banana")", make_string("monkeybanana")},
      },
  };

  run_vm_test("([vm]: String expressions)", tests);
}
