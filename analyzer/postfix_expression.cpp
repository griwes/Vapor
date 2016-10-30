/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016 Michał "Griwes" Dominiak
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation is required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 **/

#include "vapor/parser.h"
#include "vapor/analyzer/postfix_expression.h"
#include "vapor/analyzer/function.h"
#include "vapor/analyzer/id_expression.h"
#include "vapor/analyzer/helpers.h"

reaver::vapor::analyzer::_v1::postfix_expression::postfix_expression(const reaver::vapor::parser::postfix_expression & parse, reaver::vapor::analyzer::_v1::scope * lex_scope) : _parse{ parse }, _scope{ lex_scope }, _brace{ parse.bracket_type }
{
    fmap(_parse.base_expression, make_overload_set(
        [&](const parser::expression_list & expr_list){
            _base_expr = preanalyze_expression(expr_list, lex_scope);
            return unit{};
        },
        [&](const parser::id_expression & id_expr){
            _base_expr = preanalyze_id_expression(id_expr, lex_scope);
            return unit{};
        }
    ));

    _arguments = fmap(_parse.arguments, [&](auto && expr){ return preanalyze_expression(expr, lex_scope); });
}

void reaver::vapor::analyzer::_v1::postfix_expression::print(std::ostream & os, std::size_t indent) const
{
    auto in = std::string(indent, ' ');
    os << in << "postfix expression at " << _parse.range << '\n';
    os << in << "type: " << get_variable()->get_type()->explain() << '\n';
    os << in << "base expression:\n";
    os << in << "{\n";
    _base_expr->print(os, indent + 4);
    os << in << "}\n";

    if (_parse.bracket_type)
    {
        os << in << "selected overload: " << _overload->explain() << '\n';
        os << in << "bracket type: " << lexer::token_types[+_brace] << '\n';

        os << in << "arguments:\n";
        fmap(_arguments, [&](auto && arg) {
            os << in << "{\n";
            arg->print(os, indent + 4);
            os << in << "}\n";

            return unit{};
        });
    }
}

reaver::future<> reaver::vapor::analyzer::_v1::postfix_expression::_analyze()
{
    return when_all(fmap(_arguments, [&](auto && expr) {
            return expr->analyze();
        })).then([&]{
            return _base_expr->analyze();
        }).then([&]{
            if (!_parse.bracket_type)
            {
                return make_ready_future();
            }

            return resolve_overload(
                _base_expr->get_type(),
                *_parse.bracket_type,
                    fmap(_arguments, [](auto && arg) -> const type * { return arg->get_type(); }),
                _scope
            ).then([&](auto && overload) {
                    _overload = overload;
                    return _overload->return_type();
                }).then([&](auto && ret_type) {
                    this->_set_variable(make_expression_variable(this, ret_type));
                });
        });
}

std::unique_ptr<reaver::vapor::analyzer::_v1::expression> reaver::vapor::analyzer::_v1::postfix_expression::_clone_expr_with_replacement(reaver::vapor::analyzer::_v1::replacements & repl) const
{
    auto ret = std::unique_ptr<postfix_expression>(new postfix_expression(*this));

    ret->_base_expr = _base_expr->clone_expr_with_replacement(repl);
    ret->_arguments = fmap(_arguments, [&](auto && arg){ return arg->clone_expr_with_replacement(repl); });

    return ret;
}

reaver::future<reaver::vapor::analyzer::_v1::expression *> reaver::vapor::analyzer::_v1::postfix_expression::_simplify_expr(reaver::vapor::analyzer::_v1::optimization_context & ctx)
{
    return when_all(fmap(_arguments, [&](auto && expr) {
            return expr->simplify_expr(ctx);
        })).then([&](auto && simplified) {
            replace_uptrs(_arguments, simplified, ctx);
            return _base_expr->simplify_expr(ctx);
        }).then([&](auto && simplified) {
            replace_uptr(_base_expr, simplified, ctx);

            if (!_parse.bracket_type)
            {
                ctx.something_happened();
                return make_ready_future(_base_expr.release());
            }

            return _overload->simplify(ctx)
                .then([&](){
                    auto args = fmap(_arguments, [&](auto && expr){ return expr->get_variable(); });
                    return _overload->simplify(ctx, std::move(args));
                }).then([&](auto && simplified) -> expression * {
                    if (simplified)
                    {
                        ctx.something_happened();
                        return simplified;
                    }

                    return this;
                });
        });
}

reaver::vapor::analyzer::_v1::statement_ir reaver::vapor::analyzer::_v1::postfix_expression::_codegen_ir(reaver::vapor::analyzer::_v1::ir_generation_context & ctx) const
{
    auto base_expr_instructions = _base_expr->codegen_ir(ctx);

    if (!_parse.bracket_type)
    {
        return base_expr_instructions;
    }

    auto base_variable_value = base_expr_instructions.back().result;
    assert(base_variable_value.index() == 0);
    auto base_variable = get<std::shared_ptr<codegen::ir::variable>>(base_variable_value);
    auto arguments_instructions = fmap(_arguments, [&](auto && arg){ return arg->codegen_ir(ctx); });

    auto base_expr_variable = base_expr_instructions.back().result;
    auto arguments_values = fmap(arguments_instructions, [](auto && insts){ return insts.back().result; });
    arguments_values.insert(arguments_values.begin(), _overload->call_operand_ir(ctx));
    arguments_values.insert(arguments_values.begin(), std::move(base_variable_value));

    auto postfix_expr_instruction = codegen::ir::instruction{
        none, none,
        { boost::typeindex::type_id<codegen::ir::function_call_instruction>() },
        std::move(arguments_values),
        { codegen::ir::make_variable((*_overload->return_type().try_get())->codegen_type(ctx)) }
    };

    ctx.add_function_to_generate(_overload);

    statement_ir ret;
    ret.reserve(base_expr_instructions.size() + std::accumulate(
        arguments_instructions.begin(), arguments_instructions.end(),
        1, [](std::size_t i, auto && insts){ return i + insts.size(); }));
    std::move(base_expr_instructions.begin(), base_expr_instructions.end(), std::back_inserter(ret));
    fmap(arguments_instructions, [&](auto && insts) {
        std::move(insts.begin(), insts.end(), std::back_inserter(ret));
        return unit{};
    });
    ret.push_back(std::move(postfix_expr_instruction));

    return ret;
}

reaver::vapor::analyzer::_v1::variable * reaver::vapor::analyzer::_v1::postfix_expression::get_variable() const
{
    if (!_parse.bracket_type)
    {
        return _base_expr->get_variable();
    }

    return expression::get_variable();
}

