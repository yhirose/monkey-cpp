#pragma once

#include <optional>

namespace monkey {

using SymbolScope = std::string;

const SymbolScope GlobalScope = "GLOBAL";

struct Symbol {
  std::string name;
  SymbolScope scope;
  int index;

  bool operator==(const Symbol &rhs) const {
    return name == rhs.name && scope == rhs.scope && index == rhs.index;
  }
};

struct SymbolTable {
  std::map<std::string, Symbol> store;
  int numDefinitions = 0;

  const Symbol& define(const std::string& name) {
    store.emplace(name, Symbol{name, GlobalScope, numDefinitions});
    numDefinitions++;
    return store[name];
  }

  std::optional<Symbol> resolve(const std::string& name) {
    auto it = store.find(name);
    if (it != store.end()) {
      return it->second;
    }
    return std::nullopt;
  }
};

inline std::shared_ptr<SymbolTable> symbol_table() {
  return std::make_shared<SymbolTable>();
}

} // namespace monkey
