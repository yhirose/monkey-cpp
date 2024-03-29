#pragma once

#include <optional>

namespace monkey {

using SymbolScope = std::string;

const SymbolScope GlobalScope = "GLOBAL";
const SymbolScope LocalScope = "LOCAL";
const SymbolScope BuiltinScope = "BUILTIN";
const SymbolScope FreeScope = "FREE";
const SymbolScope FunctionScope = "FUNCTION";

struct Symbol {
  std::string name;
  SymbolScope scope;
  int index;

  bool operator==(const Symbol &rhs) const {
    return name == rhs.name && scope == rhs.scope && index == rhs.index;
  }
};

struct SymbolTable {
  std::shared_ptr<SymbolTable> outer;

  std::map<std::string, Symbol> store;
  int numDefinitions = 0;
  std::vector<Symbol> freeSymbols;

  const Symbol &define(const std::string &name) {
    store[name] = {
        name,
        outer ? LocalScope : GlobalScope,
        numDefinitions,
    };
    numDefinitions++;
    return store[name];
  }

  const Symbol &define_builtin(int index, const std::string &name) {
    store[name] = {
        name,
        BuiltinScope,
        index,
    };
    return store[name];
  }

  const Symbol &define_free(const Symbol &original) {
    freeSymbols.push_back(original);
    store[original.name] = {
        original.name,
        FreeScope,
        static_cast<int>(freeSymbols.size() - 1),
    };
    return store[original.name];
  }

  const Symbol &define_function_name(const std::string &name) {
    store[name] = {
        name,
        FunctionScope,
        0,
    };
    return store[name];
  }

  std::optional<Symbol> resolve(const std::string &name) {
    auto it = store.find(name);
    if (it != store.end()) {
      return it->second;
    } else if (outer) {
      auto obj = outer->resolve(name);
      if (!obj) { return obj; }
      if (obj->scope == GlobalScope || obj->scope == BuiltinScope) {
        return obj;
      }
      return define_free(*obj);
    }
    return std::nullopt;
  }
};

inline std::shared_ptr<SymbolTable> symbol_table() {
  return std::make_shared<SymbolTable>();
}

inline std::shared_ptr<SymbolTable>
enclosed_symbol_table(std::shared_ptr<SymbolTable> outer) {
  auto s = std::make_shared<SymbolTable>();
  s->outer = outer;
  return s;
}

} // namespace monkey
