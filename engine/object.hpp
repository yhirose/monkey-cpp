#pragma once

#include <ast.hpp>

namespace monkey {

struct Environment;

const std::string INTEGER_OBJ = "INTEGER";
const std::string BOOLEAN_OBJ = "BOOLEAN";
const std::string NULL_OBJ = "NULL";
const std::string ERROR_OBJ = "ERROR";
const std::string FUNCTION_OBJ = "FUNCTION";
const std::string STRING_OBJ = "STRING";
const std::string BUILTIN_OBJ = "BUILTIN";
const std::string ARRAY_OBJ = "ARRAY";
const std::string HASH_OBJ = "HASH";

struct HashKey {
  HashKey(const std::string &type, uint64_t value) : type(type), value(value) {}

  bool operator==(const HashKey &rhs) const {
    return type == rhs.type && value == rhs.value;
  }

  bool operator<(const HashKey &rhs) const {
    return type == rhs.type ? value < rhs.value : type < rhs.type;
  }

  std::string type;
  uint64_t value;
};

struct Object {
  virtual ~Object() {}
  virtual const std::string &type() const = 0;
  virtual std::string inspect() const = 0;
  virtual bool has_hash_key() const { return false; };
  virtual const HashKey &hash_key() const {
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
  const std::string &type() const override { return INTEGER_OBJ; }
  std::string inspect() const override { return std::to_string(value); }
  bool has_hash_key() const override { return true; }
  const HashKey &hash_key() const override {
    if (!hash_key_) {
      hash_key_ =
          std::make_shared<HashKey>(type(), static_cast<uint64_t>(value));
    }
    return *hash_key_;
  }

  const int64_t value;

private:
  mutable std::shared_ptr<HashKey> hash_key_;
};

struct Boolean : public Object {
  Boolean(bool value) : value(value) {}
  const std::string &type() const override { return BOOLEAN_OBJ; }
  std::string inspect() const override { return value ? "true" : "false"; }
  bool has_hash_key() const override { return true; }
  const HashKey &hash_key() const override {
    if (!hash_key_) {
      hash_key_ = std::make_shared<HashKey>(
          type(), static_cast<uint64_t>(value ? 1 : 0));
    }
    return *hash_key_;
  }

  const bool value;

private:
  mutable std::shared_ptr<HashKey> hash_key_;
};

struct Null : public Object {
  Null() {}
  const std::string &type() const override { return NULL_OBJ; }
  std::string inspect() const override { return "null"; }
};

struct Error : public Object {
  Error(const std::string &message) : message(message) {}
  const std::string &type() const override { return ERROR_OBJ; }
  std::string inspect() const override { return "ERROR: " + message; }
  const std::string message;
};

struct Function : public Object {
  Function(const std::vector<std::string> &params,
           std::shared_ptr<Environment> env, std::shared_ptr<Ast> body)
      : params(params), env(env), body(body) {}

  const std::string &type() const override { return FUNCTION_OBJ; }

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
inline uint64_t fnv1a_hash_bytes(const unsigned char *first, size_t count) {
  const uint64_t fnv_offset_basis = 14695981039346656037ULL;
  const uint64_t fnv_prime = 1099511628211ULL;

  uint64_t result = fnv_offset_basis;
  for (size_t next = 0; next < count; ++next) {
    // fold in another byte
    result ^= (size_t)first[next];
    result *= fnv_prime;
  }
  return (result);
}

struct String : public Object {
  String(const std::string &value) : value(value) {}
  const std::string &type() const override { return STRING_OBJ; }
  std::string inspect() const override { return value; }
  bool has_hash_key() const override { return true; }
  const HashKey &hash_key() const override {
    if (!hash_key_) {
      auto hash_value = fnv1a_hash_bytes(
          reinterpret_cast<const unsigned char *>(value.data()), value.size());
      hash_key_ = std::make_shared<HashKey>(type(), hash_value);
    }
    return *hash_key_;
  }

  const std::string value;

private:
  mutable std::shared_ptr<HashKey> hash_key_;
};

using Fn = std::function<std::shared_ptr<Object>(
    const std::vector<std::shared_ptr<Object>> &args)>;

struct Builtin : public Object {
  Builtin(Fn fn) : fn(fn) {}
  const std::string &type() const override { return BUILTIN_OBJ; }
  std::string inspect() const override { return "builtin function"; }
  const Fn fn;
};

struct Array : public Object {
  const std::string &type() const override { return ARRAY_OBJ; }
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
  const std::string &type() const override { return HASH_OBJ; }
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
