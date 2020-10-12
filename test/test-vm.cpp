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
    auto val = cast<Integer>(expected).value;
    test_integer_object(val, actual);
    break;
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

    auto stack_elem = vm.stack_top();

    test_expected_object(t.expected, stack_elem);
  }
}

TEST_CASE("Integer arithmetic - vm", "[vm]") {
  vector<VmTestCase> tests{
      {
          {"1", make_integer(1)},
          {"2", make_integer(2)},
          {"1 + 2", make_integer(3)},
      },
  };

  run_vm_test("([vm]: Integer arithmetic)", tests);
}

