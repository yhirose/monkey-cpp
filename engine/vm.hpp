#pragma once

#include <compiler.hpp>

namespace monkey {

struct VM {
  static const size_t StackSize = 2048;
  static const size_t GlobalSize = 65535;

  std::vector<std::shared_ptr<Object>> constants;
  Instructions instructions;

  std::vector<std::shared_ptr<Object>> stack;
  size_t sp = 0;

  std::vector<std::shared_ptr<Object>> globals;

  VM(const Bytecode &bytecode)
      : constants(bytecode.constants), instructions(bytecode.instructions),
        stack(StackSize), globals(GlobalSize) {}

  VM(const Bytecode &bytecode, const std::vector<std::shared_ptr<Object>> &s)
      : constants(bytecode.constants), instructions(bytecode.instructions),
        stack(StackSize), globals(s) {}

  std::shared_ptr<Object> stack_top() const {
    if (sp == 0) { return nullptr; }
    return stack[sp - 1];
  }

  std::shared_ptr<Object> last_popped_stack_elem() const { return stack[sp]; }

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
      case OpAdd:
      case OpSub:
      case OpMul:
      case OpDiv: execute_binary_operation(op); break;
      case OpTrue: push(CONST_TRUE); break;
      case OpFalse: push(CONST_FALSE); break;
      case OpNull: push(CONST_NULL); break;
      case OpEqual:
      case OpNotEqual:
      case OpGreaterThan: execute_comparison(op); break;
      case OpBang: execute_bang_operator(); break;
      case OpMinus: execute_minus_operator(); break;
      case OpPop: pop(); break;
      case OpJump: {
        auto pos = read_uint16(&instructions[ip + 1]);
        ip = pos - 1;
        break;
      }
      case OpJumpNotTruthy: {
        auto pos = read_uint16(&instructions[ip + 1]);
        ip += 2;
        auto condition = pop();
        if (!is_truthy(condition)) { ip = pos - 1; }
        break;
      }
      case OpSetGlobal: {
        auto globalIndex = read_uint16(&instructions[ip + 1]);
        ip += 2;
        globals[globalIndex] = pop();
        break;
      }
      case OpGetGlobal: {
        auto globalIndex = read_uint16(&instructions[ip + 1]);
        ip += 2;
        push(globals[globalIndex]);
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

  void execute_binary_operation(Opecode op) {
    auto right = pop();
    auto left = pop();

    auto left_type = left->type();
    auto right_type = right->type();

    if (left_type == INTEGER_OBJ && right_type == INTEGER_OBJ) {
      execute_binary_integer_operation(op, left, right);
      return;
    }

    throw std::runtime_error(
        fmt::format("unsupported types for binary operation: {} {}", left_type,
                    right_type));
  }

  void execute_binary_integer_operation(Opecode op,
                                        std::shared_ptr<Object> left,
                                        std::shared_ptr<Object> right) {
    auto left_value = cast<Integer>(left).value;
    auto right_value = cast<Integer>(right).value;

    int64_t result;

    switch (op) {
    case OpAdd: result = left_value + right_value; break;
    case OpSub: result = left_value - right_value; break;
    case OpMul: result = left_value * right_value; break;
    case OpDiv: result = left_value / right_value; break;
    default:
      throw std::runtime_error(fmt::format("unknown integer operator: {}", op));
    }

    push(make_integer(result));
  }

  void execute_comparison(Opecode op) {
    auto right = pop();
    auto left = pop();

    auto left_type = left->type();
    auto right_type = right->type();

    if (left_type == INTEGER_OBJ || right_type == INTEGER_OBJ) {
      execute_integer_comparison(op, left, right);
      return;
    }

    auto left_value = cast<Boolean>(left).value;
    auto right_value = cast<Boolean>(right).value;

    switch (op) {
    case OpEqual: push(make_bool(right_value == left_value)); break;
    case OpNotEqual: push(make_bool(right_value != left_value)); break;
    default:
      throw std::runtime_error(fmt::format("unknown operator: {} ({} {})", op,
                                           left_type, right_type));
    }
  }

  void execute_integer_comparison(Opecode op, std::shared_ptr<Object> left,
                                  std::shared_ptr<Object> right) {
    auto left_value = cast<Integer>(left).value;
    auto right_value = cast<Integer>(right).value;

    switch (op) {
    case OpEqual: push(make_bool(right_value == left_value)); break;
    case OpNotEqual: push(make_bool(right_value != left_value)); break;
    case OpGreaterThan: push(make_bool(left_value > right_value)); break;
    default: throw std::runtime_error(fmt::format("unknown operator: {}", op));
    }
  }

  void execute_bang_operator() {
    auto operand = pop();
    if (operand->type() == BOOLEAN_OBJ) {
      auto value = cast<Boolean>(operand).value;
      push(make_bool(!value));
    } else if (operand->type() == NULL_OBJ) {
      push(make_bool(true));
    } else {
      push(make_bool(false));
    }
  }

  void execute_minus_operator() {
    auto operand = pop();
    if (operand->type() != INTEGER_OBJ) {
      throw std::runtime_error(
          fmt::format("unsupported types for negation: {}", operand->type()));
    }

    auto value = cast<Integer>(operand).value;
    push(make_integer(value * -1));
  }

  bool is_truthy(std::shared_ptr<Object> obj) const {
    if (obj->type() == BOOLEAN_OBJ) {
      return cast<Boolean>(obj).value;
    } else if (obj->type() == NULL_OBJ) {
      return false;
    } else {
      return true;
    }
  }
};

} // namespace monkey
