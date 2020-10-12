#pragma once

#include <vector>
#include <string>

struct Options {
  bool print_ast = false;
  bool shell = false;
  bool debug = false;
  bool vm = false;
  std::vector<std::string> script_path_list;
};

inline Options parse_command_line(int argc, const char **argv) {
  Options options;

  int argi = 1;
  while (argi < argc) {
    std::string arg = argv[argi++];
    if (arg == "--shell") {
      options.shell = true;
    } else if (arg == "--ast") {
      options.print_ast = true;
    } else if (arg == "--debug") {
      options.debug = true;
    } else if (arg == "--vm") {
      options.vm = true;
    } else {
      options.script_path_list.push_back(arg);
    }
  }

  if (!options.shell) { options.shell = options.script_path_list.empty(); }

  return options;
}
