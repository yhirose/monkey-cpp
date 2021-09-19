#pragma once

#include <algorithm>
#include <fmt/core.h>
#include <map>
#include <numeric>
#include <vector>

namespace monkey {

using Instructions = std::vector<uint8_t>;

using Opecode = uint8_t;

enum OpecodeType {
  OpConstant = 0,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,
  OpTrue,
  OpFalse,
  OpNull,
  OpEqual,
  OpNotEqual,
  OpGreaterThan,
  OpMinus,
  OpBang,
  OpPop,
  OpJumpNotTruthy,
  OpJump,
  OpGetGlobal,
  OpSetGlobal,
  OpArray,
  OpHash,
  OpIndex,
  OpCall,
  OpReturnValue,
  OpReturn,
  OpGetLocal,
  OpSetLocal,
  OpGetBuiltin,
};

struct Definition {
  std::string name;
  std::vector<size_t> operand_widths;
};

inline std::map<Opecode, Definition> &definitions() {
  static auto definitions_ = std::map<Opecode, Definition>{
      {OpConstant, {"OpConstant", {2}}},
      {OpAdd, {"OpAdd", {}}},
      {OpSub, {"OpSub", {}}},
      {OpMul, {"OpMul", {}}},
      {OpDiv, {"OpDiv", {}}},
      {OpTrue, {"OpTrue", {}}},
      {OpFalse, {"OpFalse", {}}},
      {OpNull, {"OpNull", {}}},
      {OpEqual, {"OpEqual", {}}},
      {OpNotEqual, {"OpNotEqual", {}}},
      {OpGreaterThan, {"OpGreaterThan", {}}},
      {OpMinus, {"OpMinus", {}}},
      {OpBang, {"OpBang", {}}},
      {OpPop, {"OpPop", {}}},
      {OpJumpNotTruthy, {"OpJumpNotTruthy", {2}}},
      {OpJump, {"OpJump", {2}}},
      {OpGetGlobal, {"OpGetGlobal", {2}}},
      {OpSetGlobal, {"OpSetGlobal", {2}}},
      {OpArray, {"OpArray", {2}}},
      {OpHash, {"OpHash", {2}}},
      {OpIndex, {"OpIndex", {}}},
      {OpCall, {"OpCall", {1}}},
      {OpReturnValue, {"OpReturnValue", {}}},
      {OpReturn, {"OpReturn", {}}},
      {OpGetLocal, {"OpGetLocal", {1}}},
      {OpSetLocal, {"OpSetLocal", {1}}},
      {OpGetBuiltin, {"OpGetBuiltin", {1}}},
  };
  return definitions_;
}

inline Definition &lookup(Opecode op) {
  auto it = definitions().find(op);
  if (it == definitions().end()) {
    throw std::runtime_error(fmt::format("opcode {} undefined", op));
  }
  return it->second;
}

static bool is_little_endian = []() {
  short int n = 0x1;
  char *p = (char *)&n;
  return (p[0] == 1);
}();

inline void put_uint16(uint8_t *p, uint16_t n) {
  if (is_little_endian) { n = (n >> 8) | (n << 8); }
  memcpy(p, &n, sizeof(uint16_t));
}

inline uint16_t read_uint16(const uint8_t *p) {
  uint16_t n = *reinterpret_cast<const uint16_t *>(p);
  if (is_little_endian) { n = (n >> 8) | (n << 8); }
  return n;
}

inline uint8_t read_uint8(const uint8_t *p) {
  return *p;
}

inline std::vector<uint8_t> make(Opecode op, const std::vector<int> &operands) {
  auto it = definitions().find(op);
  if (it == definitions().end()) { return std::vector<uint8_t>(); }
  const auto &widths = it->second.operand_widths;

  auto instruction_len = std::accumulate(widths.begin(), widths.end(), 1);

  auto instruction = std::vector<uint8_t>(instruction_len);
  instruction[0] = op;

  size_t offset = 1;
  size_t i = 0;
  for (auto o : operands) {
    auto width = widths[i];
    switch (width) {
    case 2: put_uint16(&instruction[offset], o); break;
    case 1: instruction[offset] = static_cast<uint8_t>(o); break;
    }
    offset += width;
    i++;
  }

  return instruction;
}

inline std::pair<std::vector<int>, size_t>
read_operands(const Definition &def, const Instructions &ins,
              size_t start_offset) {
  std::vector<int> operands(def.operand_widths.size());
  size_t offset = start_offset;
  size_t i = 0;
  for (auto width : def.operand_widths) {
    switch (width) {
    case 2: operands[i] = read_uint16(&ins[offset]); break;
    case 1: operands[i] = read_uint8(&ins[offset]); break;
    }
    offset += width;
  }
  return std::pair(operands, offset - start_offset);
}

inline std::string fmt_instruction(const Definition &def,
                                   const std::vector<int> &operands) {
  auto operand_count = def.operand_widths.size();
  if (operands.size() != operand_count) {
    return fmt::format("ERROR: operand len {} does not match defined {}\n",
                       operands.size(), operand_count);
  }

  switch (operand_count) {
  case 0: return def.name;
  case 1: return fmt::format("{} {}", def.name, operands[0]);
  }

  return fmt::format("ERROR: unhandled operand count for {}\n", def.name);
}

inline std::string to_string(const Instructions &ins) {
  std::string out;

  size_t i = 0;
  while (i < ins.size()) {
    auto &def = lookup(ins[i]);
    auto [operands, read] = read_operands(def, ins, i + 1);
    out += fmt::format(R"({:04} {}\n)", i, fmt_instruction(def, operands));
    i += 1 + read;
  }

  return out;
}

} // namespace monkey
