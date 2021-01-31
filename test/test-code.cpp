#include "catch.hh"
#include "test-util.hpp"

#include <code.hpp>

using namespace std;
using namespace monkey;

TEST_CASE("Make", "[code]") {
  struct Test {
    OpecodeType op;
    vector<int> operands;
    vector<uint8_t> expected;
  };

  Test tests[] = {
      {OpConstant, {65534}, {OpConstant, 255, 254}},
      {OpAdd, {}, {OpAdd}},
  };

  for (const auto &t : tests) {
    auto instruction = make(t.op, t.operands);
    REQUIRE(instruction.size() == t.expected.size());
    CHECK(instruction == t.expected);
  }
}

TEST_CASE("Instructions string", "[code]") {
  vector<Instructions> instructions{
      make(OpAdd, {}),
      make(OpConstant, {2}),
      make(OpConstant, {65535}),
  };

  string expected = R"(0000 OpAdd\n0001 OpConstant 2\n0004 OpConstant 65535\n)";

  auto concatted = concat_instructions(instructions);

  CHECK(to_string(concatted) == expected);
}

