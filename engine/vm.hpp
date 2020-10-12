#pragma once

#include <compiler.hpp>

namespace monkey {

const size_t StackSize = 2048;

struct VM {
  std::vector<std::shared_ptr<Object>> constants;
  Instructions instructions;

  std::vector<std::shared_ptr<Object>> stack;
  size_t sp = 0;

  VM(const Bytecode &bytecode)
      : constants(bytecode.constants), instructions(bytecode.instructions),
        stack(StackSize) {}

  std::shared_ptr<Object> stack_top() const {
    if (sp == 0) { return nullptr; }
    return stack[sp - 1];
  }

  std::shared_ptr<Object> last_popped_stack_elem() const {
    return stack[sp];
  }

  void run() {
    for (size_t ip = 0; ip < instructions.size(); ip++) {
      Opecode op = instructions[ip];
      switch (op) {
      case OpConstant: {
        auto constIndex = read_uint16(&instructions[ip + 1]);
        ip += 2;
        push(constants[constIndex]);
        break;
      }
      case OpAdd: {
        auto right = pop();
        auto left = pop();
        auto left_value = cast<Integer>(left).value;
        auto right_value = cast<Integer>(right).value;
        auto result = left_value + right_value;
        push(make_integer(result));
        break;
      }
      case OpPop: {
        pop();
        break;
      }
      }
    }
  }

  void push(std::shared_ptr<Object> o) {
    if (sp >= StackSize) { throw std::runtime_error("stack overflow"); }
    stack[sp] = o;
    sp++;
  }

  std::shared_ptr<Object> pop() {
    auto o = stack[sp - 1];
    stack.pop_back();
    sp--;
    return o;
  }
};

} // namespace monkey
