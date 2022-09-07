#include "catch.hpp"
#include "test-util.hpp"

#include <object.hpp>

using namespace std;
using namespace monkey;

TEST_CASE("String hash key", "[object]") {
  auto hello1_obj = make_string("Hello World");
  auto hello2_obj = make_string("Hello World");

  auto &hello1 = cast<String>(hello1_obj);
  auto &hello2 = cast<String>(hello2_obj);
  
  auto diff1_obj = make_string("My name is johnny");
  auto diff2_obj = make_string("My name is johnny");

  auto &diff1 = cast<String>(diff1_obj);
  auto &diff2 = cast<String>(diff2_obj);

  CHECK(hello1.hash_key() == hello2.hash_key());
  CHECK(diff1.hash_key() == diff2.hash_key());
  CHECK_FALSE(hello1.hash_key() == diff1.hash_key());
}
