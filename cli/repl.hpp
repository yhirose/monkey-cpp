#pragma once

#include <evaluator.hpp>
#include <parser.hpp>

#include <vm.hpp>

#include "linenoise.hpp"

namespace monkey {

inline int repl(std::shared_ptr<monkey::Environment> env,
                const Options &options) {
  using namespace monkey;
  using namespace std;

  for (;;) {
    auto line = linenoise::Readline(">> ");

    if (line == "exit" || line == "quit") { break; }

    if (!line.empty()) {
      vector<string> msgs;
      auto ast = parse("(repl)", line.data(), line.size(), msgs);
      if (ast) {
        if (options.print_ast) { cout << peg::ast_to_s(ast); }

        try {
          if (options.vm) {
            Compiler compiler;
            compiler.compile(ast);
            VM vm(compiler.bytecode());
            vm.run();
            auto stack_top = vm.stack_top();
            cout << stack_top->inspect() << endl;
          } else {
            auto val = eval(ast, env);
            if (val->type() != ERROR_OBJ) {
              cout << val->inspect() << endl;
              linenoise::AddHistory(line.c_str());
              continue;
            } else {
              msgs.push_back(cast<Error>(val).message);
            }
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
