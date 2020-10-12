#pragma once

#include <environment.hpp>
#include <parser.hpp>

namespace monkey {

struct Evaluator {

  std::shared_ptr<Object>
  eval_bang_operator_expression(const std::shared_ptr<Object> &obj) {
    auto p = obj.get();
    if (p == CONST_TRUE.get()) {
      return CONST_FALSE;
    } else if (p == CONST_FALSE.get()) {
      return CONST_TRUE;
    } else if (p == CONST_NULL.get()) {
      return CONST_TRUE;
    }
    return CONST_FALSE;
  }

  std::shared_ptr<Object>
  eval_minus_operator_expression(const std::shared_ptr<Object> &right) {
    if (right->type() != INTEGER_OBJ) {
      throw make_error("unknown operator: -" + right->name());
    }
    auto val = cast<Integer>(right).value;
    return std::make_shared<Integer>(-val);
  }

  std::shared_ptr<Object>
  eval_prefix_expression(const Ast &node,
                         const std::shared_ptr<Environment> &env) {
    auto rit = node.nodes.rbegin();
    auto right = eval(**rit, env);
    ++rit;

    while (rit != node.nodes.rend()) {
      const auto &ope = (**rit).token;
      assert(!ope.empty());

      switch (ope[0]) {
      case '-': right = eval_minus_operator_expression(right); break;
      case '!': right = eval_bang_operator_expression(right); break;
      default:
        throw make_error("unknown operator: " + std::string(ope) +
                         right->name());
      }
      ++rit;
    }
    return right;
  }

  std::shared_ptr<Object>
  eval_integer_infix_expression(std::string_view ope,
                                const std::shared_ptr<Object> &left,
                                const std::shared_ptr<Object> &right) {
    using namespace peg::udl;

    auto tag = peg::str2tag(ope);
    auto lval = cast<Integer>(left).value;
    auto rval = cast<Integer>(right).value;

    switch (tag) {
    case "+"_: return std::make_shared<Integer>(lval + rval);
    case "-"_: return std::make_shared<Integer>(lval - rval);
    case "*"_: return std::make_shared<Integer>(lval * rval);
    case "%"_: return std::make_shared<Integer>(lval % rval);
    case "/"_:
      if (rval == 0) { throw make_error("divide by 0 error"); }
      return std::make_shared<Integer>(lval / rval);
    case "<"_: return make_bool(lval < rval);
    case ">"_: return make_bool(lval > rval);
    case "=="_: return make_bool(lval == rval);
    case "!="_: return make_bool(lval != rval);
    default:
      throw make_error("unknown operator: " + left->name() + " " +
                       std::string(ope) + " " + right->name());
    }
  }

  std::shared_ptr<Object>
  eval_string_infix_expression(std::string_view &ope,
                               const std::shared_ptr<Object> &left,
                               const std::shared_ptr<Object> &right) {
    using namespace peg::udl;

    auto tag = peg::str2tag(ope);

    if (tag != "+"_) {
      throw make_error("unknown operator: " + left->name() + " " +
                       std::string(ope) + " " + right->name());
    }

    auto lval = cast<String>(left).value;
    auto rval = cast<String>(right).value;
    return make_string(lval + rval);
  }

  std::shared_ptr<Object>
  eval_infix_expression(const Ast &node,
                        const std::shared_ptr<Environment> &env) {
    using namespace peg::udl;

    auto left = eval(*node.nodes[0], env);
    auto ope = node.nodes[1]->token;
    auto right = eval(*node.nodes[2], env);

    if (left->type() == INTEGER_OBJ && right->type() == INTEGER_OBJ) {
      return eval_integer_infix_expression(ope, left, right);
    } else if (left->type() == STRING_OBJ && right->type() == STRING_OBJ) {
      return eval_string_infix_expression(ope, left, right);
    }

    auto tag = peg::str2tag(ope);

    switch (tag) {
    case "=="_: return make_bool(left.get() == right.get());
    case "!="_: return make_bool(left.get() != right.get());
    }

    if (left->type() != right->type()) {
      throw make_error("type mismatch: " + left->name() + " " +
                       std::string(ope) + " " + right->name());
    }

    throw make_error("unknown operator: " + left->name() + " " +
                     std::string(ope) + " " + right->name());
  }

  std::shared_ptr<Object>
  eval_statements(const Ast &node, const std::shared_ptr<Environment> &env) {
    if (node.is_token) {
      return eval(node, env);
    } else if (node.nodes.empty()) {
      return CONST_NULL;
    }
    auto it = node.nodes.begin();
    while (it != node.nodes.end() - 1) {
      auto obj = eval(**it, env);
      if (obj->type() == ObjectType::RETURN_OBJ) { return obj; }
      ++it;
    }
    return eval(**it, env);
  }

  std::shared_ptr<Object> eval_block(const Ast &node,
                                     const std::shared_ptr<Environment> &env) {
    auto scopeEnv = std::make_shared<Environment>(env);
    return eval(*node.nodes[0], scopeEnv);
  }

  bool is_truthy(const std::shared_ptr<Object> &obj) {
    auto p = obj.get();
    if (p == CONST_NULL.get()) {
      return false;
    } else if (p == CONST_TRUE.get()) {
      return true;
    } else if (p == CONST_FALSE.get()) {
      return false;
    }
    return true;
  }

  std::shared_ptr<Object> eval_if(const Ast &node,
                                  const std::shared_ptr<Environment> &env) {
    const auto &nodes = node.nodes;
    auto cond = eval(*nodes[0], env);
    if (is_truthy(cond)) {
      return eval(*nodes[1], env);
    } else if (nodes.size() == 3) {
      return eval(*nodes[2], env);
    }
    return CONST_NULL;
  }

  std::shared_ptr<Object> eval_return(const Ast &node,
                                      const std::shared_ptr<Environment> &env) {
    return std::make_shared<Return>(eval(*node.nodes[0], env));
  }

  std::shared_ptr<Object>
  eval_assignment(const Ast &node, const std::shared_ptr<Environment> &env) {
    auto ident = node.nodes[0]->token;
    auto rval = eval(*node.nodes.back(), env);
    env->set(ident, rval);
    return rval;
  };

  std::shared_ptr<Object>
  eval_identifier(const Ast &node, const std::shared_ptr<Environment> &env) {
    return env->get(node.token, [&]() {
      throw make_error("identifier not found: " + std::string(node.token));
    });
  };

  std::shared_ptr<Object>
  eval_function(const Ast &node, const std::shared_ptr<Environment> &env) {
    std::vector<std::string_view> params;
    for (auto node : node.nodes[0]->nodes) {
      params.push_back(node->token);
    }
    auto body = node.nodes[1];
    return std::make_shared<Function>(params, env, body);
  };

  std::shared_ptr<Object>
  eval_function_call(const Ast &node, const std::shared_ptr<Environment> &env,
                     const std::shared_ptr<Object> &left) {
    if (left->type() == BUILTIN_OBJ) {
      const auto &builtin = cast<Builtin>(left);
      std::vector<std::shared_ptr<Object>> args;
      for (auto arg : node.nodes) {
        args.emplace_back(eval(*arg, env));
      }
      try {
        return builtin.fn(args);
      } catch (const std::shared_ptr<Object> &e) { return e; }
    }

    const auto &fn = cast<Function>(left);
    const auto &args = node.nodes;

    if (fn.params.size() <= args.size()) {
      auto callEnv = std::make_shared<Environment>(fn.env);
      for (auto iprm = 0u; iprm < fn.params.size(); iprm++) {
        auto name = fn.params[iprm];
        auto arg = args[iprm];
        auto val = eval(*arg, env);
        callEnv->set(name, val);
      }

      auto obj = eval(*fn.body, callEnv);
      if (obj->type() == ObjectType::RETURN_OBJ) {
        return cast<Return>(obj).value;
      }
      return obj;
    }

    return make_error("arguments error...");
  }

  std::shared_ptr<Object>
  eval_array_index_expression(const std::shared_ptr<Object> &left,
                              const std::shared_ptr<Object> &index) {
    const auto &arr = cast<Array>(left);
    auto idx = cast<Integer>(index).value;
    if (0 <= idx && idx < static_cast<int64_t>(arr.elements.size())) {
      return arr.elements[idx];
    } else {
      return CONST_NULL;
    }
    return left;
  }

  std::shared_ptr<Object>
  eval_hash_index_expression(const std::shared_ptr<Object> &left,
                             const std::shared_ptr<Object> &index) {
    const auto &hash = cast<Hash>(left);
    if (!index->has_hash_key()) {
      throw make_error("unusable as hash key: " + index->name());
    }
    auto hashed = index->hash_key();
    auto it = hash.pairs.find(hashed);
    if (it == hash.pairs.end()) { return CONST_NULL; }
    const auto &pair = it->second;
    return pair.value;
  }

  std::shared_ptr<Object>
  eval_index_expression(const Ast &node,
                        const std::shared_ptr<Environment> &env,
                        const std::shared_ptr<Object> &left) {
    auto index = eval(node, env);
    switch (left->type()) {
    case ARRAY_OBJ: return eval_array_index_expression(left, index);
    case HASH_OBJ: return eval_hash_index_expression(left, index);
    default: return make_error("index operator not supported: " + left->name());
    }
  }

  std::shared_ptr<Object> eval_call(const Ast &node,
                                    const std::shared_ptr<Environment> &env) {
    using namespace peg::udl;

    auto left = eval(*node.nodes[0], env);

    for (auto i = 1u; i < node.nodes.size(); i++) {
      const auto &postfix = *node.nodes[i];

      switch (postfix.original_tag) {
      case "ARGUMENTS"_: left = eval_function_call(postfix, env, left); break;
      case "INDEX"_:
        left = eval_index_expression(*postfix.nodes[0], env, left);
        break;
      default: throw std::logic_error("invalid internal condition.");
      }
    }

    return left;
  }

  std::shared_ptr<Object> eval_array(const Ast &node,
                                     const std::shared_ptr<Environment> &env) {
    auto arr = std::make_shared<Array>();
    const auto &nodes = node.nodes;
    for (auto i = 0u; i < nodes.size(); i++) {
      auto expr = nodes[i];
      auto val = eval(*expr, env);
      if (i < arr->elements.size()) {
        arr->elements[i] = std::move(val);
      } else {
        arr->elements.push_back(std::move(val));
      }
    }
    return arr;
  }

  std::shared_ptr<Object> eval_hash(const Ast &node,
                                    const std::shared_ptr<Environment> &env) {
    auto hash = std::make_shared<Hash>();
    for (auto i = 0u; i < node.nodes.size(); i++) {
      const auto &pair = *node.nodes[i];
      auto key = eval(*pair.nodes[0], env);
      if (!key->has_hash_key()) {
        throw make_error("unusable as hash key: " + key->name());
      }
      auto hashed = key->hash_key();
      auto value = eval(*pair.nodes[1], env);
      hash->pairs.emplace(hashed, HashPair{key, value});
    }
    return hash;
  }

  std::shared_ptr<Object> eval(const Ast &node,
                               const std::shared_ptr<Environment> &env) {
    using namespace peg::udl;

    switch (node.tag) {
    case "INTEGER"_: return std::make_shared<Integer>(node.to_integer());
    case "BOOLEAN"_: return make_bool(node.to_bool());
    case "PREFIX_EXPR"_: return eval_prefix_expression(node, env);
    case "INFIX_EXPR"_: return eval_infix_expression(node, env);
    case "STATEMENTS"_: return eval_statements(node, env);
    case "BLOCK"_: return eval_block(node, env);
    case "IF"_: return eval_if(node, env);
    case "RETURN"_: return eval_return(node, env);
    case "ASSIGNMENT"_: return eval_assignment(node, env);
    case "EXPRESSION_STATEMENT"_: return eval(*node.nodes[0], env);
    case "IDENTIFIER"_: return eval_identifier(node, env);
    case "FUNCTION"_: return eval_function(node, env);
    case "CALL"_: return eval_call(node, env);
    case "ARRAY"_: return eval_array(node, env);
    case "HASH"_: return eval_hash(node, env);
    }

    if (node.is_token) { return make_string(node.token); }

    // NOTREACHED
    throw std::logic_error("invalid Ast type: " + node.name);
  }
};

inline std::shared_ptr<Object> eval(const std::shared_ptr<Ast> &ast,
                                    const std::shared_ptr<Environment> &env) {
  try {
    auto obj = Evaluator().eval(*ast, env);
    if (obj->type() == ObjectType::RETURN_OBJ) {
      return cast<Return>(obj).value;
    }
    return obj;
  } catch (const std::shared_ptr<Object> &err) { return err; }
  return CONST_NULL;
}

} // namespace monkey
