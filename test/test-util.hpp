#include <parser.hpp>

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
