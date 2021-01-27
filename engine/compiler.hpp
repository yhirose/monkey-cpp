#pragma once

#include <code.hpp>
#include <object.hpp>
#include <symbol_table.hpp>

namespace monkey {

struct EmittedInstruction {
  Opecode opecode;
  int position;
};

struct Bytecode {
  Instructions instructions;
  std::vector<std::shared_ptr<Object>> constants;
};

struct Compiler {
  std::shared_ptr<SymbolTable> symbolTable;
  std::vector<std::shared_ptr<Object>> constants;

  Instructions instructions;
  EmittedInstruction lastInstruction;
  EmittedInstruction previousInstruction;

  Compiler() : symbolTable(symbol_table()){};

  Compiler(std::shared_ptr<SymbolTable> symbolTable,
           const std::vector<std::shared_ptr<Object>> &constants)
      : symbolTable(symbolTable), constants(constants){};

  void compile(const std::shared_ptr<Ast> &ast) {
    using namespace peg::udl;

    switch (ast->tag) {
    case "STATEMENTS"_: {
      for (auto node : ast->nodes) {
        compile(node);
      }
      break;
    }
    case "ASSIGNMENT"_: {
      compile(ast->nodes[1]);
      auto name = std::string(ast->nodes[0]->token);
      auto symbol = symbolTable->define(name);
      emit(OpSetGlobal, {symbol.index});
      break;
    }
    case "IDENTIFIER"_: {
      auto name = std::string(ast->token);
      const auto &symbol = symbolTable->resolve(name);
      if (!symbol) {
        throw std::runtime_error(fmt::format("undefined variable {}", name));
      }
      emit(OpGetGlobal, {symbol.value().index});
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
    case "PREFIX_EXPR"_: {
      int i = ast->nodes.size() - 1;
      compile(ast->nodes[i--]);

      while (i >= 0) {
        auto op = ast->nodes[i]->token;
        switch (peg::str2tag(op)) {
        case "!"_: emit(OpBang, {}); break;
        case "-"_: emit(OpMinus, {}); break;
        default:
          throw std::runtime_error(fmt::format("unknown operator {}", op));
          break;
        }
        i--;
      }
      break;
    }
    case "IF"_: {
      compile(ast->nodes[0]);

      // Emit an `OpJumpNotTruthy` with a bogus value
      auto jump_not_truthy_pos = emit(OpJumpNotTruthy, {9999});

      // Consequence
      compile(ast->nodes[1]);

      if (last_instruction_is_pop()) { remove_last_pop(); }

      // Emit an `OpJump` with a bogus value
      auto jump_pos = emit(OpJump, {9999});

      auto after_consequence_pos = instructions.size();
      change_operand(jump_not_truthy_pos, after_consequence_pos);

      if (ast->nodes.size() < 3) {
        // Has no alternative
        emit(OpNull, {});
      } else {
        // Alternative
        compile(ast->nodes[2]);

        if (last_instruction_is_pop()) { remove_last_pop(); }
      }

      auto after_alternative_pos = instructions.size();
      change_operand(jump_pos, after_alternative_pos);
      break;
    }
    case "BLOCK"_: {
      compile(ast->nodes[0]);
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
    case "STRING"_: {
      auto str = std::make_shared<String>(ast->token);
      emit(OpConstant, {add_constant(str)});
      break;
    }
    case "ARRAY"_: {
      for (auto node : ast->nodes) {
        compile(node);
      }
      emit(OpArray, {static_cast<int>(ast->nodes.size())});
      break;
    }
    case "HASH"_: {
      for (auto node : ast->nodes) {
        compile(node->nodes[0]);
        compile(node->nodes[1]);
      }
      emit(OpHash, {static_cast<int>(ast->nodes.size() * 2)});
      break;
    }
    case "CALL"_: {
      compile(ast->nodes[0]);
      for (auto i = 1u; i < ast->nodes.size(); i++) {
        auto postfix = ast->nodes[i];
        switch (postfix->original_tag) {
        case "INDEX"_: compile(postfix->nodes[0]); break;
        }
        emit(OpIndex, {});
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
    set_last_instruction(op, pos);
    return pos;
  }

  size_t add_instruction(const std::vector<uint8_t> &ins) {
    auto pos_new_instruction = instructions.size();
    instructions.insert(instructions.end(), ins.begin(), ins.end());
    return pos_new_instruction;
  }

  void set_last_instruction(Opecode op, int pos) {
    auto previous = lastInstruction;
    auto last = EmittedInstruction{op, pos};
    previousInstruction = previous;
    lastInstruction = last;
  }

  bool last_instruction_is_pop() const {
    return lastInstruction.opecode == OpPop;
  }

  void remove_last_pop() {
    instructions.pop_back();
    lastInstruction = previousInstruction;
  }

  void replace_instruction(int pos,
                           const std::vector<uint8_t> &new_instructions) {
    for (size_t i = 0; i < new_instructions.size(); i++) {
      instructions[pos + i] = new_instructions[i];
    }
  }

  void change_operand(int op_pos, int operand) {
    auto op = instructions[op_pos];
    auto new_instruction = make(op, {operand});
    replace_instruction(op_pos, new_instruction);
  }

  Bytecode bytecode() { return Bytecode{instructions, constants}; }
};

} // namespace monkey
