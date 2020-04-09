#pragma once

#include <ast.hpp>

namespace monkey {

struct Environment;

enum ObjectType {
  INTEGER_OBJ = 0,
  BOOLEAN_OBJ,
  NULL_OBJ,
  ERROR_OBJ,
  FUNCTION_OBJ,
  STRING_OBJ,
  BUILTIN_OBJ,
  ARRAY_OBJ,
  HASH_OBJ
};

struct HashKey {
  HashKey(ObjectType type, uint64_t value) : type(type), value(value) {}
  bool operator==(const HashKey &rhs) const {
    return type == rhs.type && value == rhs.value;
  }
  bool operator<(const HashKey &rhs) const {
    return type == rhs.type ? value < rhs.value : type < rhs.type;
  }

  ObjectType type;
  uint64_t value;
};

struct Object {
  virtual ~Object() {}
  virtual ObjectType type() const = 0;
  virtual std::string name() const = 0;
  virtual std::string inspect() const = 0;
  virtual bool has_hash_key() const { return false; };
  virtual HashKey hash_key() const {
    throw std::logic_error("invalid internal condition.");
  };

protected:
  Object() = default;
};

template <typename T> inline T &cast(std::shared_ptr<Object> obj) {
  return dynamic_cast<T &>(*obj);
}

struct Integer : public Object {
  Integer(int64_t value) : value(value) {}
  ObjectType type() const override { return INTEGER_OBJ; }
  std::string name() const override { return "INTEGER"; }
  std::string inspect() const override { return std::to_string(value); }
  bool has_hash_key() const override { return true; }
  HashKey hash_key() const override {
    return HashKey{type(), static_cast<uint64_t>(value)};
  }

  const int64_t value;
};

struct Boolean : public Object {
  Boolean(bool value) : value(value) {}
  ObjectType type() const override { return BOOLEAN_OBJ; }
  std::string name() const override { return "BOOLEAN"; }
  std::string inspect() const override { return value ? "true" : "false"; }
  bool has_hash_key() const override { return true; }
  HashKey hash_key() const override {
    return HashKey{type(), static_cast<uint64_t>(value ? 1 : 0)};
  }

  const bool value;
};

struct Null : public Object {
  Null() {}
  ObjectType type() const override { return NULL_OBJ; }
  std::string name() const override { return "NULL"; }
  std::string inspect() const override { return "null"; }
};

struct Error : public Object {
  Error(const std::string &message) : message(message) {}
  ObjectType type() const override { return ERROR_OBJ; }
  std::string name() const override { return "ERROR"; }
  std::string inspect() const override { return "ERROR: " + message; }
  const std::string message;
};

struct Function : public Object {
  Function(const std::vector<std::string> &params,
           std::shared_ptr<Environment> env, std::shared_ptr<Ast> body)
      : params(params), env(env), body(body) {}

  ObjectType type() const override { return FUNCTION_OBJ; }
  std::string name() const override { return "FUNCTION"; }

  std::string inspect() const override {
    std::stringstream ss;
    ss << "fn(";
    for (size_t i = 0; i < params.size(); i++) {
      if (i != 0) { ss << ", "; }
      ss << params[i];
    }
    ss << ") {\n";
    ss << to_string(body);
    ss << "\n}";
    return ss.str();
  }

  const std::vector<std::string> params;
  const std::shared_ptr<Environment> env;
  const std::shared_ptr<Ast> body;
};

// https://docs.microsoft.com/en-us/cpp/porting/fix-your-dependencies-on-library-internals?view=vs-2019
inline uint64_t fnv1a_hash_bytes(const char *first, size_t count) {
  const auto fnv_offset_basis = 14695981039346656037ULL;
  const auto fnv_prime = 1099511628211ULL;

  auto result = fnv_offset_basis;
  for (size_t next = 0; next < count; ++next) {
    // fold in another byte
    result ^= static_cast<uint64_t>(first[next]);
    result *= fnv_prime;
  }
  return result;
}

struct String : public Object {
  String(const std::string &value) : value(value) {}
  ObjectType type() const override { return STRING_OBJ; }
  std::string name() const override { return "STRING"; }
  std::string inspect() const override { return value; }
  bool has_hash_key() const override { return true; }
  HashKey hash_key() const override {
    auto hash_value = fnv1a_hash_bytes(value.data(), value.size());
    return HashKey{type(), hash_value};
  }

  const std::string value;
};

using Fn = std::function<std::shared_ptr<Object>(
    const std::vector<std::shared_ptr<Object>> &args)>;

struct Builtin : public Object {
  Builtin(Fn fn) : fn(fn) {}
  ObjectType type() const override { return BUILTIN_OBJ; }
  std::string name() const override { return "BUILTIN"; }
  std::string inspect() const override { return "builtin function"; }
  const Fn fn;
};

struct Array : public Object {
  ObjectType type() const override { return ARRAY_OBJ; }
  std::string name() const override { return "ARRAY"; }
  std::string inspect() const override {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < elements.size(); i++) {
      if (i != 0) { ss << ", "; }
      ss << elements[i]->inspect();
    }
    ss << "]";
    return ss.str();
  }

  std::vector<std::shared_ptr<Object>> elements;
};

struct HashPair {
  std::shared_ptr<Object> key;
  std::shared_ptr<Object> value;
};

struct Hash : public Object {
  ObjectType type() const override { return HASH_OBJ; }
  std::string name() const override { return "HASH"; }
  std::string inspect() const override {
    std::stringstream ss;
    ss << "{";
    auto it = pairs.begin();
    for (size_t i = 0; i < pairs.size(); i++) {
      if (i != 0) { ss << ", "; }
      auto [key, value] = it->second;
      ss << key->inspect();
      ss << ": ";
      ss << value->inspect();
      ++it;
    }
    ss << "}";
    return ss.str();
  }

  std::map<HashKey, HashPair> pairs;
};

inline std::shared_ptr<Object> make_integer(int64_t n) {
  return std::make_shared<Integer>(n);
}

inline std::shared_ptr<Object> make_error(const std::string &s) {
  return std::make_shared<Error>(s);
}

inline std::shared_ptr<Object> make_string(const std::string &s) {
  return std::make_shared<String>(s);
}

inline std::shared_ptr<Object> make_builtin(Fn fn) {
  return std::make_shared<Builtin>(fn);
}

inline std::shared_ptr<Object> make_array(std::vector<int64_t> numbers) {
  auto arr = std::make_shared<Array>();
  for (auto n : numbers) {
    arr->elements.emplace_back(make_integer(n));
  }
  return arr;
}

const std::shared_ptr<Object> CONST_TRUE = std::make_shared<Boolean>(true);
const std::shared_ptr<Object> CONST_FALSE = std::make_shared<Boolean>(false);
const std::shared_ptr<Object> CONST_NULL = std::make_shared<Null>();

inline std::shared_ptr<Object> make_bool(bool value) {
  return value ? CONST_TRUE : CONST_FALSE;
}

} // namespace monkey
