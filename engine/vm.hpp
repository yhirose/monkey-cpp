#pragma once

#include <compiler.hpp>

namespace monkey {

struct Frame {
  std::shared_ptr<Closure> cl;
  int ip = -1;
  int basePointer = -1;

  Frame(std::shared_ptr<Closure> cl, int basePointer)
      : cl(cl), basePointer(basePointer) {}

  const Instructions &instructions() const { return cl->fn->instructions; }
};

struct VM {
  static const size_t StackSize = 2048;
  static const size_t GlobalSize = 65535;
  static const size_t MaxFrames = 1024;

  std::vector<std::shared_ptr<Object>> constants;

  std::vector<std::shared_ptr<Object>> stack;
  size_t sp = 0;

  std::vector<std::shared_ptr<Object>> globals;

  std::vector<std::shared_ptr<Frame>> frames;
  int framesIndex = 1;

  VM(const Bytecode &bytecode)
      : constants(bytecode.constants), stack(StackSize), globals(GlobalSize),
        frames(MaxFrames) {
    auto mainFn = std::make_shared<CompiledFunction>(bytecode.instructions);
    auto mainClosure = std::make_shared<Closure>(mainFn);
    frames[0] = std::make_shared<Frame>(mainClosure, 0);
  }

  VM(const Bytecode &bytecode, const std::vector<std::shared_ptr<Object>> &s)
      : constants(bytecode.constants), stack(StackSize), globals(s),
        frames(MaxFrames) {
    auto mainFn = std::make_shared<CompiledFunction>(bytecode.instructions);
    auto mainClosure = std::make_shared<Closure>(mainFn);
    frames[0] = std::make_shared<Frame>(mainClosure, 0);
  }

  std::shared_ptr<Object> stack_top() const {
    if (sp == 0) { return nullptr; }
    return stack[sp - 1];
  }

  std::shared_ptr<Object> last_popped_stack_elem() const { return stack[sp]; }

  std::shared_ptr<Frame> current_frame() const {
    return frames[framesIndex - 1];
  }

  void push_frame(std::shared_ptr<Frame> f) {
    frames[framesIndex] = f;
    framesIndex++;
  }

  std::shared_ptr<Frame> pop_frame() {
    framesIndex--;
    return frames[framesIndex];
  }

  void run() {
    try {
      while (current_frame()->ip <
             static_cast<int>(current_frame()->instructions().size()) - 1) {
        current_frame()->ip++;

        auto ip = current_frame()->ip;
        const auto &ins = current_frame()->instructions();
        Opecode op = ins[ip];

        switch (op) {
        case OpConstant: {
          auto constIndex =
              read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;
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
          auto pos = read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip = pos - 1;
          break;
        }
        case OpJumpNotTruthy: {
          auto pos = read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;
          auto condition = pop();
          if (!is_truthy(condition)) { current_frame()->ip = pos - 1; }
          break;
        }
        case OpSetGlobal: {
          auto globalIndex =
              read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;
          globals[globalIndex] = pop();
          break;
        }
        case OpGetGlobal: {
          auto globalIndex =
              read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;
          push(globals[globalIndex]);
          break;
        }
        case OpArray: {
          auto numElements =
              read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;

          auto array = build_array(sp - numElements, sp);
          sp = sp - numElements;
          push(array);
          break;
        }
        case OpHash: {
          auto numElements =
              read_uint16(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 2;

          auto hash = build_hash(sp - numElements, sp);
          sp = sp - numElements;
          push(hash);
          break;
        }
        case OpIndex: {
          auto index = pop();
          auto left = pop();
          execute_index_expression(left, index);
          break;
        }
        case OpCall: {
          auto numArgs = read_uint8(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 1;
          execute_call(numArgs);
          break;
        }
        case OpReturnValue: {
          auto returnValue = pop();
          auto frame = pop_frame();
          sp = frame->basePointer - 1;
          push(returnValue);
          break;
        }
        case OpReturn: {
          auto frame = pop_frame();
          sp = frame->basePointer - 1;
          push(CONST_NULL);
          break;
        }
        case OpSetLocal: {
          auto localIndex =
              read_uint8(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 1;
          auto frame = current_frame();
          stack[frame->basePointer + localIndex] = pop();
          break;
        }
        case OpGetLocal: {
          auto localIndex =
              read_uint8(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 1;
          auto frame = current_frame();
          push(stack[frame->basePointer + localIndex]);
          break;
        }
        case OpGetBuiltin: {
          auto builtinIndex = read_uint8(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 1;
          auto definition = BUILTINS[builtinIndex];
          push(definition.second);
          break;
        }
        case OpClosure: {
          auto constIndex = read_uint16(&current_frame()->instructions()[ip + 1]);
          auto numFree = read_uint8(&current_frame()->instructions()[ip + 3]);
          current_frame()->ip += 3;
          push_closure(constIndex, numFree);
          break;
        }
        case OpGetFree: {
          auto freeIndex = read_uint8(&current_frame()->instructions()[ip + 1]);
          current_frame()->ip += 1;
          auto currentClosure = current_frame()->cl;
          push(currentClosure->free[freeIndex]);
          break;
        }
        case OpCurrentClosure: {
          auto currentClosure = current_frame()->cl;
          push(currentClosure);
          break;
        }
        }
      }
    } catch (const std::shared_ptr<Object> &err) {
      push(err);
      pop();
    }
  }

  void push(std::shared_ptr<Object> o) {
    if (sp >= StackSize) { throw make_error("stack overflow"); }
    stack[sp] = o;
    sp++;
  }

  void push_closure(int constIndex, int numFree) {
    auto constant = constants[constIndex];
    auto function = std::dynamic_pointer_cast<CompiledFunction>(constant);
    if (!function) {
      throw make_error(fmt::format("not a function: {}", constIndex));
    }

    std::vector<std::shared_ptr<Object>> free;
    for (int i = 0; i < numFree; i++) {
      free.push_back(stack[sp - numFree + i]);
    }
    sp = sp - numFree;

    auto closure = std::make_shared<Closure>(function, free);
    push(closure);
  }

  std::shared_ptr<Object> pop() {
    auto o = stack[sp - 1];
    stack.pop_back();
    sp--;
    return o;
  }

  void call_closure(std::shared_ptr<Closure> cl, int numArgs) {
    if (numArgs != cl->fn->numParameters) {
      throw make_error(fmt::format("wrong number of arguments: want={}, got={}",
                                   cl->fn->numParameters, numArgs));
    }
    auto frame = std::make_shared<Frame>(cl, sp - numArgs);
    push_frame(frame);
    sp = frame->basePointer + cl->fn->numLocals;
  }

  void call_builtin(std::shared_ptr<Builtin> builtin, int numArgs) {
    std::vector<std::shared_ptr<Object>> args;
    for (int i = 0; i < numArgs; i++) {
      args.push_back(stack[sp - (numArgs - i)]);
    }
    auto result = builtin->fn(args);
    sp = sp - numArgs;
    push(result);
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

    if (left_type == STRING_OBJ && right_type == STRING_OBJ) {
      execute_binary_string_operation(op, left, right);
      return;
    }

    throw make_error(
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
    default: throw make_error(fmt::format("unknown integer operator: {}", op));
    }

    push(make_integer(result));
  }

  void execute_binary_string_operation(Opecode op, std::shared_ptr<Object> left,
                                       std::shared_ptr<Object> right) {
    auto left_value = cast<String>(left).value;
    auto right_value = cast<String>(right).value;

    if (op != OpAdd) {
      throw make_error(fmt::format("unknown integer operator: {}", op));
    }

    push(make_string(left_value + right_value));
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
      throw make_error(fmt::format("unknown operator: {} ({} {})", op,
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
    default: make_error(fmt::format("unknown operator: {}", op));
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
      throw make_error(
          fmt::format("unsupported types for negation: {}", operand->type()));
    }

    auto value = cast<Integer>(operand).value;
    push(make_integer(value * -1));
  }

  void execute_index_expression(std::shared_ptr<Object> left,
                                std::shared_ptr<Object> index) {
    if (left->type() == ARRAY_OBJ && index->type() == INTEGER_OBJ) {
      return execute_array_index(left, index);
    } else if (left->type() == HASH_OBJ) {
      return execute_hash_index(left, index);
    } else {
      throw make_error(
          fmt::format("index operator not supported: {}", left->type()));
    }
  }

  void execute_array_index(std::shared_ptr<Object> array,
                           std::shared_ptr<Object> index) {
    auto &arrayObject = cast<Array>(array);
    auto i = cast<Integer>(index).value;
    int64_t max = arrayObject.elements.size() - 1;
    if (i < 0 || i > max) {
      push(CONST_NULL);
      return;
    }
    push(arrayObject.elements[i]);
  }

  void execute_hash_index(std::shared_ptr<Object> hash,
                          std::shared_ptr<Object> index) {
    auto &hashObject = cast<Hash>(hash);
    auto key = index->hash_key();
    auto it = hashObject.pairs.find(key);
    if (it == hashObject.pairs.end()) {
      push(CONST_NULL);
      return;
    }
    push(it->second.value);
  }

  void execute_call(int numArgs) {
    auto callee = stack[sp - 1 - numArgs];
    if (callee) {
      if (callee->type() == CLOSURE_OBJ) {
        call_closure(std::dynamic_pointer_cast<Closure>(callee), numArgs);
        return;
      } else if (callee->type() == BUILTIN_OBJ) {
        call_builtin(std::dynamic_pointer_cast<Builtin>(callee), numArgs);
        return;
      }
    }
    throw make_error("calling non-function and non-built-in");
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

  std::shared_ptr<Object> build_array(int startIndex, int endIndex) {
    auto arr = std::make_shared<Array>();
    for (auto i = startIndex; i < endIndex; i++) {
      arr->elements.push_back(std::move(stack[i]));
    }
    return arr;
  }

  std::shared_ptr<Object> build_hash(int startIndex, int endIndex) {
    auto hash = std::make_shared<Hash>();
    for (auto i = startIndex; i < endIndex; i += 2) {
      auto key = stack[i];
      auto value = stack[i + 1];
      hash->pairs[key->hash_key()] = HashPair{key, value};
    }
    return hash;
  }
};

} // namespace monkey
