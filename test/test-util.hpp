#include <code.hpp>
#include <object.hpp>
#include <parser.hpp>

#include <fmt/core.h>

inline std::shared_ptr<monkey::Ast> parse(const char *name,
                                          const std::string &input) {
  using namespace std;
  using namespace monkey;

  vector<string> msgs;
  auto ast = parse(name, input.data(), input.size(), msgs);
  for (auto msg : msgs) {
    cerr << msg << endl;
  }
  return ast;
}

inline void test_integer_object(int64_t expected,
                                std::shared_ptr<monkey::Object> actual) {
  using namespace monkey;

  REQUIRE(actual);
  CHECK(actual->type() == INTEGER_OBJ);

  auto val = cast<Integer>(actual).value;
  CHECK(val == expected);
}

inline void test_boolean_object(bool expected,
                                std::shared_ptr<monkey::Object> actual) {
  using namespace monkey;

  REQUIRE(actual);
  CHECK(actual->type() == BOOLEAN_OBJ);

  auto val = cast<Boolean>(actual).value;
  CHECK(val == expected);
}

inline void test_null_object(std::shared_ptr<monkey::Object> actual) {
  using namespace monkey;

  REQUIRE(actual);
  CHECK(actual->type() == NULL_OBJ);

  CHECK(actual.get() == CONST_NULL.get());
}

inline monkey::Instructions
concat_instructions(const std::vector<monkey::Instructions> &s) {
  monkey::Instructions out;
  for (const auto &ins : s) {
    out.insert(out.end(), ins.begin(), ins.end());
  }
  return out;
}

