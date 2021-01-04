#include "catch.hh"
#include "test-util.hpp"

#include <symbol_table.hpp>

using namespace std;
using namespace monkey;

TEST_CASE("Define", "[symbol table]") {
  map<string, Symbol> expected{
      {"a", {"a", GlobalScope, 0}},
      {"b", {"b", GlobalScope, 1}},
  };

  SymbolTable global;

  auto a = global.define("a");
  CHECK(a == expected["a"]);

  auto b = global.define("b");
  CHECK(b == expected["b"]);
}

TEST_CASE("Resolve Global", "[symbol table]") {
  SymbolTable global;
  global.define("a");
  global.define("b");

  Symbol expected[] = {
      {"a", GlobalScope, 0},
      {"b", GlobalScope, 1},
  };

  for (const auto &sym : expected) {
    auto result = global.resolve(sym.name);
    CHECK(result.has_value());
    CHECK(result.value() == sym);
  }
}
