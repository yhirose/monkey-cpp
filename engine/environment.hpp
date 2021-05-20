#pragma once

#include <map>
#include <object.hpp>

namespace monkey {

struct Environment {
  Environment(std::shared_ptr<Environment> outer = nullptr)
      : level(outer ? outer->level + 1 : 0), outer(outer) {}

  std::shared_ptr<Object> get(std::string_view sv,
                              std::function<void(void)> error_handler) const {
    auto s = std::string(sv);
    if (dictionary.find(s) != dictionary.end()) {
      return dictionary.at(s);
    } else if (outer) {
      return outer->get(s, error_handler);
    }
    if (error_handler) { error_handler(); }
    // NOTREACHED
    throw std::logic_error("invalid internal condition.");
  }

  void set(std::string_view sv, std::shared_ptr<Object> val) {
    dictionary[std::string(sv)] = std::move(val);
  }

  size_t level;
  std::shared_ptr<Environment> outer;
  std::map<std::string, std::shared_ptr<Object>> dictionary;
};

inline void setup_built_in_functions(Environment &env) {
  env.set("len", BUILTINS.at("len"));
  env.set("puts", BUILTINS.at("puts"));
  env.set("first", BUILTINS.at("first"));
  env.set("last", BUILTINS.at("last"));
  env.set("rest", BUILTINS.at("rest"));
  env.set("push", BUILTINS.at("push"));
}

inline std::shared_ptr<Environment> environment() {
  auto env = std::make_shared<Environment>();
  setup_built_in_functions(*env);
  return env;
}

} // namespace monkey
