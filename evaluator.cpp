// evaluator.cpp
#include "calc.h"
#include <cmath>
#include <cctype>
#include <algorithm>

using namespace calc;
using namespace std;

//   Context
   
   void Context::set_var(const string &name, double value) {
    vars[name] = value;
}

optional<double> Context::get_var(const string &name) const {
    auto it = vars.find(name);
    if (it == vars.end()) return {};
    return it->second;
}

// Evaluator
   
Evaluator::Evaluator(Context &ctx) : ctx_(ctx) {}

Result<double> Evaluator::eval(const ASTPtr &node)
{
    if (!node) return Result<double>::Err("Null AST node");

    switch (node->type) {
        case ASTNode::Type::Number:   return eval_number(node);
        case ASTNode::Type::Variable: return eval_variable(node);
        case ASTNode::Type::Binary:   return eval_binary(node);
        case ASTNode::Type::Unary:    return eval_unary(node);
        case ASTNode::Type::Function: return eval_function(node);
        case ASTNode::Type::Assign:   return eval_assign(node);
    }

    return Result<double>::Err("Unknown AST node type");
}

// Number parsing
   
Result<double> Evaluator::parse_number_text(const string &text)
{
    string s = text;

    // Data cleanup
    s.erase(remove(s.begin(), s.end(), '_'), s.end());

    /* ---- Binary: postfix b / B (e.g. 1100b) ---- */
    if (s.size() > 1 && (s.back() == 'b' || s.back() == 'B')) {
        double value = 0;

        for (size_t i = 0; i < s.size() - 1; ++i) {
            char c = s[i];
            if (c == '0' || c == '1') {
                value = value * 2 + (c - '0');
            } else {
                return Result<double>::Err(
                    "Invalid binary digit in " + s
                );
            }
        }
        return Result<double>::Ok(value);
    }

    /* ---- Hexadecimal: prefix 0x / 0X ---- */
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        double value = 0;

        for (size_t i = 2; i < s.size(); ++i) {
            char c = tolower(s[i]);
            if (isdigit(c)) {
                value = value * 16 + (c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value = value * 16 + (10 + c - 'a');
            } else {
                return Result<double>::Err(
                    "Invalid hexadecimal digit in " + s
                );
            }
        }
        return Result<double>::Ok(value);
    }
    
    try {
        return Result<double>::Ok(stod(s));
    }
    catch (...) {
        return Result<double>::Err("Invalid numeric literal: " + s);
    }
}

// valuation helpers
   
Result<double> Evaluator::eval_number(const ASTPtr &node)
{
    if (!node->number_text.empty())
        return parse_number_text(node->number_text);

    return Result<double>::Ok(node->number);
}

Result<double> Evaluator::eval_variable(const ASTPtr &node)
{
    auto v = ctx_.get_var(node->name);
    if (!v.has_value())
        return Result<double>::Err("Undefined variable: " + node->name);

    return Result<double>::Ok(*v);
}

Result<double> Evaluator::eval_unary(const ASTPtr &node)
{
    auto val_r = eval(node->left);
    if (!val_r.ok()) return val_r;

    double v = val_r.value;
    if (node->op == '-') return Result<double>::Ok(-v);
    if (node->op == '+') return Result<double>::Ok(v);

    return Result<double>::Err("Unknown unary operator");
}

Result<double> Evaluator::eval_binary(const ASTPtr &node)
{
    auto a_r = eval(node->left);  if (!a_r.ok()) return a_r;
    auto b_r = eval(node->right); if (!b_r.ok()) return b_r;

    double a = a_r.value, b = b_r.value;

    switch (node->op) {
        case '+': return Result<double>::Ok(a + b);
        case '-': return Result<double>::Ok(a - b);
        case '*': return Result<double>::Ok(a * b);
        case '/':
            if (b == 0) return Result<double>::Err("Division by zero");
            return Result<double>::Ok(a / b);
        case '^': return Result<double>::Ok(pow(a, b));
        default:
            return Result<double>::Err("Unknown binary operator");
    }
}

Result<double> Evaluator::eval_function(const ASTPtr &node)
{
    string fname = node->name;
    transform(fname.begin(), fname.end(), fname.begin(),
              [](unsigned char c){ return tolower(c); });

    if (node->args.size() != 1)
        return Result<double>::Err("Function expects one argument: " + fname);

    auto arg_r = eval(node->args[0]);
    if (!arg_r.ok()) return arg_r;

    double a = arg_r.value;

    if (fname == "sin") return Result<double>::Ok(sin(a));
    if (fname == "cos") return Result<double>::Ok(cos(a));

    return Result<double>::Err("Unknown function: " + fname);
}

Result<double> Evaluator::eval_assign(const ASTPtr &node)
{
    auto rhs = eval(node->right);
    if (!rhs.ok()) return rhs;

    ctx_.set_var(node->name, rhs.value);
    return Result<double>::Ok(rhs.value);
}
