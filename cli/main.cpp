#include "repl.hpp"
#include "run.hpp"

int main(int argc, const char **argv) {
  auto options = parse_command_line(argc, argv);

  try {
    auto env = monkey::environment();

    if (!run(env, options)) { return -1; }

    if (options.shell) { repl(env, options.print_ast); }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
