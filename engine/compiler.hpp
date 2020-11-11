#pragma once

#include <code.hpp>
#include <object.hpp>

namespace monkey {

struct Bytecode {
  Instructions instructions;
  std::vector<std::shared_ptr<Object>> constants;
};

struct Compiler {
  Instructions instructions;
  std::vector<std::shared_ptr<Object>> constants;

  void compile(const std::shared_ptr<Ast> &ast) {
    using namespace peg::udl;

    switch (ast->tag) {
    case "STATEMENTS"_: {
      for (auto node : ast->nodes) {
        compile(node);
      }
      break;
    }
    case "EXPRESSION_STATEMENT"_: {
      compile(ast->nodes[0]);
      emit(OpPop, {});
      break;
    }
    case "INFIX_EXPR"_: {
      auto op = ast->nodes[1]->token;

      if (op == "<") {
        compile(ast->nodes[2]);
        compile(ast->nodes[0]);
        emit(OpGreaterThan, {});
        return;
      }

      compile(ast->nodes[0]);
      compile(ast->nodes[2]);

      switch (peg::str2tag(op)) {
      case "+"_: emit(OpAdd, {}); break;
      case "-"_: emit(OpSub, {}); break;
      case "*"_: emit(OpMul, {}); break;
      case "/"_: emit(OpDiv, {}); break;
      case ">"_: emit(OpGreaterThan, {}); break;
      case "=="_: emit(OpEqual, {}); break;
      case "!="_: emit(OpNotEqual, {}); break;
      default:
        throw std::runtime_error(fmt::format("unknown operator {}", op));
        break;
      }

      break;
    }
    case "INTEGER"_: {
      auto integer = std::make_shared<Integer>(ast->to_integer());
      emit(OpConstant, {add_constant(integer)});
      break;
    }
    case "BOOLEAN"_: {
      if (ast->to_bool()) {
        emit(OpTrue, {});
      } else {
        emit(OpFalse, {});
      }
      break;
    }
    }
  }

  int add_constant(std::shared_ptr<Object> obj) {
    constants.push_back(obj);
    return constants.size() - 1;
  }

  size_t emit(Opecode op, const std::vector<int> &operands) {
    auto ins = make(op, operands);
    auto pos = add_instruction(ins);
    return pos;
  }

  size_t add_instruction(const std::vector<uint8_t> &ins) {
    auto pos_new_instruction = instructions.size();
    instructions.insert(instructions.end(), ins.begin(), ins.end());
    return pos_new_instruction;
  }

  Bytecode bytecode() { return Bytecode{instructions, constants}; }
};

} // namespace monkey
