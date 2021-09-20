#include "catch.hh"
#include "test-util.hpp"

#include <symbol_table.hpp>

using namespace std;
using namespace monkey;

TEST_CASE("Define", "[symbol table]") {
  auto expected = map<string, Symbol>{
      {"a", {"a", GlobalScope, 0}}, {"b", {"b", GlobalScope, 1}},
      {"c", {"c", LocalScope, 0}},  {"d", {"d", LocalScope, 1}},
      {"e", {"e", LocalScope, 0}},  {"f", {"f", LocalScope, 1}},
  };

  auto global = symbol_table();

  auto a = global->define("a");
  CHECK(a == expected["a"]);

  auto b = global->define("b");
  CHECK(b == expected["b"]);

  auto firstLocal = enclosed_symbol_table(global);

  auto c = firstLocal->define("c");
  CHECK(c == expected["c"]);

  auto d = firstLocal->define("d");
  CHECK(d == expected["d"]);

  auto secondLocal = enclosed_symbol_table(firstLocal);

  auto e = secondLocal->define("e");
  CHECK(e == expected["e"]);

  auto f = secondLocal->define("f");
  CHECK(f == expected["f"]);
}

TEST_CASE("Resolve Global", "[symbol table]") {
  auto global = symbol_table();
  global->define("a");
  global->define("b");

  Symbol expected[] = {
      {"a", GlobalScope, 0},
      {"b", GlobalScope, 1},
  };

  for (const auto &sym : expected) {
    auto result = global->resolve(sym.name);
    CHECK(result.has_value());
    CHECK(result.value() == sym);
  }
}

TEST_CASE("Resolve Local", "[symbol table]") {
  auto global = symbol_table();
  global->define("a");
  global->define("b");

  auto local = enclosed_symbol_table(global);
  local->define("c");
  local->define("d");

  Symbol expected[] = {
      {"a", GlobalScope, 0},
      {"b", GlobalScope, 1},
      {"c", LocalScope, 0},
      {"d", LocalScope, 1},
  };

  for (const auto &sym : expected) {
    auto result = local->resolve(sym.name);
    CHECK(result.has_value());
    CHECK(result.value() == sym);
  }
}

TEST_CASE("Resolve Nested Local", "[symbol table]") {
  auto global = symbol_table();
  global->define("a");
  global->define("b");

  auto firstLocal = enclosed_symbol_table(global);
  firstLocal->define("c");
  firstLocal->define("d");

  auto secondLocal = enclosed_symbol_table(firstLocal);
  secondLocal->define("e");
  secondLocal->define("f");

  pair<shared_ptr<SymbolTable>, vector<Symbol>> tests[] = {
      {
          firstLocal,
          {
              {"a", GlobalScope, 0},
              {"b", GlobalScope, 1},
              {"c", LocalScope, 0},
              {"d", LocalScope, 1},
          },
      },
      {
          secondLocal,
          {
              {"a", GlobalScope, 0},
              {"b", GlobalScope, 1},
              {"e", LocalScope, 0},
              {"f", LocalScope, 1},
          },
      },
  };

  for (const auto &[table, expectedSymbols] : tests) {
    for (const auto &sym : expectedSymbols) {
      auto result = table->resolve(sym.name);
      CHECK(result.has_value());
      CHECK(result.value() == sym);
    }
  }
}

TEST_CASE("Define Resolve Builtins", "[symbol table]") {
  auto global = symbol_table();
  auto firstLocal = enclosed_symbol_table(global);
  auto secondLocal = enclosed_symbol_table(firstLocal);

  vector<Symbol> expected = {
      {"a", BuiltinScope, 0},
      {"c", BuiltinScope, 1},
      {"e", BuiltinScope, 2},
      {"f", BuiltinScope, 3},
  };

  size_t i = 0;
  for (const auto &v : expected) {
    global->define_builtin(i, v.name);
    i++;
  }

  auto tables = std::vector<std::shared_ptr<SymbolTable>>{global, firstLocal,
                                                          secondLocal};

  for (auto table : tables) {
    for (const auto &sym : expected) {
      auto result = table->resolve(sym.name);
      CHECK(result.has_value());
      CHECK(result.value() == sym);
    }
  }
}

TEST_CASE("Resolve Free", "[symbol table]") {
  auto global = symbol_table();
  global->define("a");
  global->define("b");

  auto firstLocal = enclosed_symbol_table(global);
  firstLocal->define("c");
  firstLocal->define("d");

  auto secondLocal = enclosed_symbol_table(firstLocal);
  secondLocal->define("e");
  secondLocal->define("f");

  tuple<shared_ptr<SymbolTable>, vector<Symbol>, vector<Symbol>> tests[] = {
      {firstLocal,
       {
           {"a", GlobalScope, 0},
           {"b", GlobalScope, 1},
           {"c", LocalScope, 0},
           {"d", LocalScope, 1},
       },
       {}},
      {secondLocal,
       {
           {"a", GlobalScope, 0},
           {"b", GlobalScope, 1},
           {"c", FreeScope, 0},
           {"d", FreeScope, 1},
           {"e", LocalScope, 0},
           {"f", LocalScope, 1},
       },
       {
           {"c", LocalScope, 0},
           {"d", LocalScope, 1},
       }},
  };

  for (const auto &[table, expectedSymbols, expectedFreeSymbols] : tests) {
    for (const auto &sym : expectedSymbols) {
      auto result = table->resolve(sym.name);
      CHECK(result.has_value());
      CHECK(result.value() == sym);
    }

    CHECK(table->freeSymbols.size() == expectedFreeSymbols.size());

    size_t i = 0;
    for (const auto &sym : expectedFreeSymbols) {
      auto result = table->freeSymbols[i];
      CHECK(result == sym);
      i++;
    }
  }
}

TEST_CASE("Resolve Unresolvable Free", "[symbol table]") {
  auto global = symbol_table();
  global->define("a");

  auto firstLocal = enclosed_symbol_table(global);
  firstLocal->define("c");

  auto secondLocal = enclosed_symbol_table(firstLocal);
  secondLocal->define("e");
  secondLocal->define("f");

  auto expected = vector<Symbol>{
      {"a", GlobalScope, 0},
      {"c", FreeScope, 0},
      {"e", LocalScope, 0},
      {"f", LocalScope, 1},
  };

  for (const auto &sym : expected) {
    auto result = secondLocal->resolve(sym.name);
    CHECK(result.has_value());
    CHECK(result.value() == sym);
  }

  auto expectedUnresolvable = vector<string>{
      "b",
      "d",
  };

  for (const auto &name : expectedUnresolvable) {
    auto result = secondLocal->resolve(name);
    CHECK(!result.has_value());
  }
}

TEST_CASE("Define and Resolve Function Name", "[symbol table]") {
  auto global = symbol_table();
  global->define_function_name("a");

  auto expected = Symbol{"a", FunctionScope, 0};

  auto result = global->resolve(expected.name);
  CHECK(result.has_value());
  CHECK(result.value() == expected);
}

TEST_CASE("Shadowing Function Name", "[symbol table]") {
  auto global = symbol_table();
  global->define_function_name("a");
  global->define("a");

  auto expected = Symbol{"a", GlobalScope, 0};

  auto result = global->resolve(expected.name);
  CHECK(result.has_value());
  CHECK(result.value().name == expected.name);
  CHECK(result.value().scope == expected.scope);
  CHECK(result.value().index == expected.index);
}
