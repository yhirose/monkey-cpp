#pragma once

#include <evaluator.hpp>
#include <parser.hpp>

#include "linenoise.hpp"

namespace monkey {

inline int repl(std::shared_ptr<monkey::Environment> env, bool print_ast) {
  using namespace monkey;
  using namespace std;

  for (;;) {
    auto line = linenoise::Readline(">> ");

    if (line == "exit" || line == "quit") { break; }

    if (!line.empty()) {
      vector<string> msgs;
      auto ast = parse("(repl)", line.data(), line.size(), msgs);
      if (ast) {
        if (print_ast) { cout << peg::ast_to_s(ast); }

        try {
          auto val = eval(ast, env);
          if (val->type() != ERROR_OBJ) {
            cout << val->inspect() << endl;
            linenoise::AddHistory(line.c_str());
            continue;
          } else {
            msgs.push_back(cast<Error>(val).message);
          }
        } catch (const std::exception &e) { msgs.push_back(e.what()); }
      }

      for (const auto &msg : msgs) {
        cout << msg << endl;
      }
    }
  }

  return 0;
}

} // namespace monkey
