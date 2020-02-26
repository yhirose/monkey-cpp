#include "catch.hh"
#include "test-util.hpp"

#include <object.hpp>

using namespace std;
using namespace monkey;

TEST_CASE("String hash key", "[object]") {
  auto hello1 = cast<String>(make_string("Hello World"));
  auto hello2 = cast<String>(make_string("Hello World"));
  auto diff1 = cast<String>(make_string("My name is johnny"));
  auto diff2 = cast<String>(make_string("My name is johnny"));

  CHECK(hello1.hash_key() == hello2.hash_key());
  CHECK(diff1.hash_key() == diff2.hash_key());
  CHECK_FALSE(hello1.hash_key() == diff1.hash_key());
}
