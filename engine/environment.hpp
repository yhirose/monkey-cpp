#pragma once

#include <map>
#include <object.hpp>

namespace monkey {

struct Environment {
  Environment(std::shared_ptr<Environment> outer = nullptr)
      : level(outer ? outer->level + 1 : 0), outer(outer) {}

  std::shared_ptr<Object> get(std::string_view s,
                              std::function<void(void)> error_handler) const {
    if (dictionary.find(s) != dictionary.end()) {
      return dictionary.at(s);
    } else if (outer) {
      return outer->get(s, error_handler);
    }
    if (error_handler) { error_handler(); }
    // NOTREACHED
    throw std::logic_error("invalid internal condition.");
  }

  void set(std::string_view s, std::shared_ptr<Object> val) {
    dictionary[s] = std::move(val);
  }

  size_t level;
  std::shared_ptr<Environment> outer;
  std::map<std::string_view, std::shared_ptr<Object>> dictionary;
};

inline void
validate_args_for_array(const std::vector<std::shared_ptr<Object>> &args,
                        const std::string &name, size_t argc) {
  if (args.size() != argc) {
    std::stringstream ss;
    ss << "wrong number of arguments. got=" << args.size() << ", want=" << argc;
    throw make_error(ss.str());
  }

  auto arg = args[0];
  if (arg->type() != ARRAY_OBJ) {
    std::stringstream ss;
    ss << "argument to `" << name << "` must be ARRAY, got " << arg->name();
    throw make_error(ss.str());
  }
}

inline void setup_built_in_functions(Environment &env) {
  env.set(
      "len", make_builtin([](const std::vector<std::shared_ptr<Object>> &args) {
        if (args.size() != 1) {
          std::stringstream ss;
          ss << "wrong number of arguments. got=" << args.size() << ", want=1";
          throw make_error(ss.str());
        }
        auto arg = args[0];
        switch (arg->type()) {
        case STRING_OBJ: {
          const auto &s = cast<String>(arg).value;
          return make_integer(s.size());
        }
        case ARRAY_OBJ: {
          const auto &arr = cast<Array>(arg);
          return make_integer(arr.elements.size());
        }
        default: {
          std::stringstream ss;
          ss << "argument to `len` not supported, got " << arg->name();
          throw make_error(ss.str());
        }
        }
      }));

  env.set("first",
          make_builtin([](const std::vector<std::shared_ptr<Object>> &args) {
            validate_args_for_array(args, "first", 1);
            const auto &elements = cast<Array>(args[0]).elements;
            if (elements.empty()) { return CONST_NULL; }
            return elements.front();
          }));

  env.set("last",
          make_builtin([](const std::vector<std::shared_ptr<Object>> &args)
                           -> std::shared_ptr<Object> {
            validate_args_for_array(args, "last", 1);
            const auto &elements = cast<Array>(args[0]).elements;
            if (elements.empty()) { return CONST_NULL; }
            return elements.back();
          }));

  env.set("rest",
          make_builtin([](const std::vector<std::shared_ptr<Object>> &args)
                           -> std::shared_ptr<Object> {
            validate_args_for_array(args, "rest", 1);
            const auto &elements = cast<Array>(args[0]).elements;
            if (!elements.empty()) {
              auto arr = std::make_shared<Array>();
              arr->elements.assign(elements.begin() + 1, elements.end());
              return arr;
            }
            return CONST_NULL;
          }));

  env.set("push",
          make_builtin([](const std::vector<std::shared_ptr<Object>> &args) {
            validate_args_for_array(args, "push", 2);
            const auto &elements = cast<Array>(args[0]).elements;
            auto arr = std::make_shared<Array>();
            arr->elements = elements;
            arr->elements.emplace_back(args[1]);
            return arr;
          }));

  env.set("puts",
          make_builtin([](const std::vector<std::shared_ptr<Object>> &args) {
            for (auto arg : args) {
              std::cout << arg->inspect() << std::endl;
            }
            return CONST_NULL;
          }));
}

inline std::shared_ptr<Environment> environment() {
  auto env = std::make_shared<Environment>();
  setup_built_in_functions(*env);
  return env;
}

} // namespace monkey
