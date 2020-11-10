#include "catch.hh"
#include "test-util.hpp"

#include <compiler.hpp>
#include <vm.hpp>

using namespace std;
using namespace monkey;

void test_expected_object(shared_ptr<Object> expected,
                          shared_ptr<Object> actual) {
  if (expected->type() == INTEGER_OBJ) {
    auto val = cast<Integer>(expected).value;
    test_integer_object(val, actual);
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
      },
  };

  run_vm_test("([vm]: Integer arithmetic)", tests);
}

