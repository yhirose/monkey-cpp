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
      {OpGetLocal, {255}, {OpGetLocal, 255}},
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
      make(OpGetLocal, {1}),
      make(OpConstant, {2}),
      make(OpConstant, {65535}),
  };

  string expected = R"(0000 OpAdd\n0001 OpGetLocal 1\n0003 OpConstant 2\n0006 OpConstant 65535\n)";

  auto concatted = concat_instructions(instructions);

  CHECK(to_string(concatted) == expected);
}

