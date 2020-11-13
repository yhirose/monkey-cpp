#pragma once

#include <evaluator.hpp>
#include <fstream>
#include <parser.hpp>

inline bool read_file(const char *path, std::vector<char> &buff) {
  using namespace std;

  ifstream ifs(path, ios::in | ios::binary);
  if (ifs.fail()) { return false; }

  auto size = static_cast<unsigned int>(ifs.seekg(0, ios::end).tellg());

  if (size > 0) {
    buff.resize(size);
    ifs.seekg(0, ios::beg).read(&buff[0], static_cast<streamsize>(buff.size()));
  }

  return true;
}

inline bool run(std::shared_ptr<monkey::Environment> env,
                const Options &options) {
  using namespace monkey;
  using namespace std;

  for (auto path : options.script_path_list) {
    vector<char> buff;
    if (!read_file(path.c_str(), buff)) {
      cerr << "can't open '" << path << "'." << endl;
      return false;
    }

    vector<string> msgs;
    auto ast = parse(path, buff.data(), buff.size(), msgs);

    if (ast) {
      if (options.print_ast) { cout << peg::ast_to_s(ast); }

      auto val = eval(ast, env);
      if (val->type() != ERROR_OBJ) {
        continue;
      } else {
        msgs.push_back(cast<Error>(val).message);
      }
    }

    for (const auto &msg : msgs) {
      cerr << msg << endl;
    }
    return false;
  }

  return true;
}
