#pragma once

#include <peglib.h>

namespace monkey {

struct Annotation {
  peg::any value;

  bool to_bool() const { return peg::any_cast<bool>(value); }
  int64_t to_integer() const { return peg::any_cast<int64_t>(value); }
};

using Ast = peg::AstBase<Annotation>;

inline void annotate(std::shared_ptr<Ast> &ast) {
  using namespace peg::udl;

  if (ast->is_token) {
    assert(ast->nodes.empty());
    switch (ast->tag) {
    case "BOOLEAN"_: ast->value = (ast->token == "true"); break;
    case "INTEGER"_: ast->value = stoll(ast->token); break;
    case "STRING"_: ast->value = ast->token; break;
    }
  } else {
    for (auto node : ast->nodes) {
      annotate(node);
    }
  }
}

std::string to_string(const std::shared_ptr<Ast> &ast);

inline std::string list_to_string(const std::shared_ptr<Ast> &ast) {
  std::string out;
  const auto &nodes = ast->nodes;
  for (size_t i = 0; i < nodes.size(); i++) {
    if (i > 0) { out += ", "; }
    out += to_string(nodes[i]);
  }
  return out;
}

inline std::string to_string(const std::shared_ptr<Ast> &ast) {
  using namespace peg::udl;

  if (ast->is_token) {
    return ast->token;
  } else {
    std::string out;
    switch (ast->tag) {
    case "ASSIGNMENT"_: {
      out = "let " + ast->nodes[0]->token + " = " + to_string(ast->nodes[1]) +
            ";";
      break;
    }
    case "PREFIX_EXPR"_: {
      auto rit = ast->nodes.rbegin();
      out = to_string(*rit);
      ++rit;
      while (rit != ast->nodes.rend()) {
        auto ope = (**rit).token;
        out = '(' + ope + out + ')';
        ++rit;
      }
      break;
    }
    case "INFIX_EXPR"_:
      out = '(' + to_string(ast->nodes[0]) + ' ' + ast->nodes[1]->token + ' ' +
            to_string(ast->nodes[2]) + ')';
      break;
    case "CALL"_: {
      auto it = ast->nodes.begin();
      out = to_string(*it);
      ++it;
      while (it != ast->nodes.end()) {
        if ((**it).tag == "INDEX"_) {
          out = '(' + out + to_string(*it) + ')';
        } else {
          out = out + to_string(*it);
        }
        ++it;
      }
      break;
    }
    case "ARGUMENTS"_: {
      out = '(' + list_to_string(ast) + ')';
      break;
    }
    case "INDEX"_: {
      out = '[' + to_string(ast->nodes[0]) + ']';
      break;
    }
    case "FUNCTION"_: {
      out = "fn" + to_string(ast->nodes[0]) + '{' + to_string(ast->nodes[1]) +
            '}';
      break;
    }
    case "PARAMETERS"_: {
      out = '(' + list_to_string(ast) + ')';
      break;
    }
    case "ARRAY"_: {
      out = '[' + list_to_string(ast) + ']';
      break;
    }
    case "HASH"_: {
      out = '{' + list_to_string(ast) + '}';
      break;
    }
    case "HASH_PAIR"_: {
      out = to_string(ast->nodes[0]) + ": " + to_string(ast->nodes[1]);
      break;
    }
    default:
      for (auto node : ast->nodes) {
        out += to_string(node);
      }
      break;
    }
    return out;
  }
}

} // namespace monkey
