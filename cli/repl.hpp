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

  std::vector<std::shared_ptr<Object>> constants;
  std::vector<std::shared_ptr<Object>> globals(VM::GlobalSize);
  std::shared_ptr<SymbolTable> symbolTable = symbol_table();

  {
    int i = 0;
    for (const auto &[name, _] : BUILTINS) {
      symbolTable->define_builtin(i, name);
      i++;
    }
  }

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
            Compiler compiler(symbolTable, constants);
            compiler.compile(ast);
            constants = compiler.constants;
            VM vm(compiler.bytecode(), globals);
            vm.run();
            globals = vm.globals;
            auto last_poped = vm.last_popped_stack_elem();
            cout << last_poped->inspect() << endl;
            linenoise::AddHistory(line.c_str());
          } else {
            auto val = eval(ast, env);
            if (val->type() != ERROR_OBJ) {
              cout << val->inspect() << endl;
              linenoise::AddHistory(line.c_str());
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
