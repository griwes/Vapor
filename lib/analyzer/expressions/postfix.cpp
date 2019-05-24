/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2019 Michał "Griwes" Dominiak
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

#include <reaver/prelude/fold.h>

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/expression_list.h"
#include "vapor/analyzer/expressions/identifier.h"
#include "vapor/analyzer/expressions/member.h"
#include "vapor/analyzer/expressions/postfix.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/overloads.h"
#include "vapor/parser.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<postfix_expression> preanalyze_postfix_expression(precontext & ctx,
        const parser::postfix_expression & parse,
        scope * lex_scope)
    {
        return std::make_unique<postfix_expression>(make_node(parse),
            std::get<0>(fmap(parse.base_expression,
                make_overload_set(
                    [&](const parser::expression_list & expr_list) -> std::unique_ptr<expression> {
                        return preanalyze_expression_list(ctx, expr_list, lex_scope);
                    },
                    [&](const parser::identifier & ident) -> std::unique_ptr<expression> {
                        return preanalyze_identifier(ctx, ident, lex_scope);
                    },
                    [&](auto &&) -> std::unique_ptr<expression> { assert(0); }))),
            parse.modifier_type,
            fmap(parse.arguments, [&](auto && expr) { return preanalyze_expression(ctx, expr, lex_scope); }),
            fmap(parse.accessed_member, [&](auto && member) { return member.value.string; }));
    }

    postfix_expression::postfix_expression(ast_node parse,
        std::unique_ptr<expression> base,
        std::optional<lexer::token_type> mod,
        std::vector<std::unique_ptr<expression>> arguments,
        std::optional<std::u32string> accessed_member)
        : _base_expr{ std::move(base) },
          _modifier{ mod },
          _arguments{ std::move(arguments) },
          _accessed_member{ std::move(accessed_member) }
    {
        _set_ast_info(parse);
    }

    void postfix_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "postfix-expression";
        print_address_range(os, this);
        os << '\n';

        auto type_ctx = ctx.make_branch(false);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        get_type()->print(os, type_ctx.make_branch(true));

        auto base_expr_ctx = ctx.make_branch(!_modifier);
        os << styles::def << base_expr_ctx << styles::subrule_name << "base expression:\n";
        _base_expr->print(os, base_expr_ctx.make_branch(true));

        if (_modifier)
        {
            if (_modifier == lexer::token_type::dot)
            {
                auto referenced_ctx = ctx.make_branch(true);
                os << styles::def << referenced_ctx << styles::subrule_name
                   << "referenced member: " << styles::string_value << utf8(*_accessed_member) << '\n';
                return;
            }

            auto modifier_ctx = ctx.make_branch(false);
            os << styles::def << modifier_ctx << styles::subrule_name
               << "modifier type: " << styles::string_value << lexer::token_types[+_modifier.value()] << '\n';

            if (_arguments.size())
            {
                auto arguments_ctx = ctx.make_branch(false);
                os << styles::def << arguments_ctx << styles::subrule_name << "arguments:\n";

                std::size_t idx = 0;
                for (auto && argument : _arguments)
                {
                    argument->print(os, arguments_ctx.make_branch(++idx == _arguments.size()));
                }
            }

            auto call_expr_ctx = ctx.make_branch(true);
            os << styles::def << call_expr_ctx << styles::subrule_name << "resolved expression:\n";
            _call_expression->print(os, call_expr_ctx.make_branch(true));
        }
    }

    template<typename Self>
    auto postfix_expression::_get_replacement_helper(Self && self)
    {
        if (self._accessed_member)
        {
            return self._referenced_expression.value()->_get_replacement();
        }

        if (self._modifier)
        {
            return self._call_expression->_get_replacement();
        }

        return self._base_expr->_get_replacement();
    }

    expression * postfix_expression::_get_replacement()
    {
        auto repl = _get_replacement_helper(*this);
        assert(repl);
        return repl;
    }

    const expression * postfix_expression::_get_replacement() const
    {
        auto repl = _get_replacement_helper(*this);
        assert(repl);
        return repl;
    }

    future<> postfix_expression::_analyze(analysis_context & ctx)
    {
        return _base_expr->analyze(ctx)
            .then([&] {
                auto expr_ctx = get_context();
                expr_ctx.push_back(this);

                return when_all(fmap(_arguments, [&](auto && expr) {
                    expr->set_context(expr_ctx);
                    return expr->analyze(ctx);
                }));
            })
            .then([&] {
                if (!_modifier)
                {
                    this->_set_type(_base_expr->get_type());
                    return make_ready_future();
                }

                if (_modifier == lexer::token_type::dot)
                {
                    return _base_expr->get_type()
                        ->get_scope()
                        ->get(*_accessed_member)
                        ->get_expression_future()
                        .then([&](auto && var) {
                            _referenced_expression = var;
                            return _referenced_expression.value()->analyze(ctx);
                        })
                        .then([&] { this->_set_type(_referenced_expression.value()->get_type()); });
                }

                return resolve_overload(ctx,
                    get_ast_info()->range,
                    _base_expr.get(),
                    *_modifier,
                    fmap(_arguments, [](auto && arg) { return arg.get(); }))
                    .then([&](std::unique_ptr<expression> call_expr) {
                        if (auto call_expr_downcasted = call_expr->as<call_expression>())
                        {
                            call_expr_downcasted->set_ast_info(get_ast_info().value());
                        }
                        _call_expression = std::move(call_expr);
                        return _call_expression->analyze(ctx);
                    })
                    .then([&] {
                        this->_set_type(_call_expression->get_type());
                        if (has_entity_name())
                        {
                            _call_expression->set_name(get_entity_name());
                        }
                    });
            });
    }

    std::unique_ptr<expression> postfix_expression::_clone_expr(replacements & repl) const
    {
        if (_call_expression)
        {
            return repl.claim(_call_expression.get());
        }

        auto base = repl.claim(_base_expr.get());
        if (!_modifier)
        {
            return base;
        }

        assert(_arguments.empty());
        auto ret = std::unique_ptr<postfix_expression>(
            new postfix_expression(get_ast_info().value(), std::move(base), _modifier, {}, _accessed_member));

        auto type = ret->_base_expr->get_type();

        ret->_referenced_expression = fmap(_referenced_expression, [&](auto && expr) {
            if (auto replaced = repl.try_get_replacement(expr))
            {
                type = replaced->get_type();
                return replaced;
            }
            type = expr->get_type();
            return expr;
        });

        if (auto new_type = repl.try_get_replacement(type))
        {
            type = new_type;
        }
        ret->_set_type(type);

        return ret;
    }

    future<expression *> postfix_expression::_simplify_expr(recursive_context ctx)
    {
        if (_call_expression)
        {
            replacements repl;
            auto clone = repl.claim(_call_expression.get()).release();

            return clone->simplify_expr(ctx).then([ctx, clone](auto && simplified) {
                if (simplified && simplified != clone)
                {
                    ctx.proper.keep_alive(clone);
                    return simplified;
                }
                return clone;
            });
        }

        return when_all(fmap(_arguments, [&, ctx](auto && expr) { return expr->simplify_expr(ctx); }))
            .then([&, ctx](auto && simplified) {
                replace_uptrs(_arguments, simplified, ctx.proper);
                return _base_expr->simplify_expr(ctx);
            })
            .then([&, ctx](auto && simplified) {
                replace_uptr(_base_expr, simplified, ctx.proper);

                if (!_modifier)
                {
                    if (has_entity_name())
                    {
                        _base_expr->set_name(get_entity_name());
                    }
                    return make_ready_future(_base_expr.release());
                }

                if (_accessed_member)
                {
                    if (!_referenced_expression.value()->is_member())
                    {
                        return _referenced_expression.value()->simplify_expr(ctx).then(
                            [](auto && simpl) { return make_expression_ref(simpl, std::nullopt).release(); });
                    }

                    if (!_base_expr->is_constant())
                    {
                        return make_ready_future<expression *>(this);
                    }

                    assert(_referenced_expression.value()->is_member());
                    auto member = _base_expr->get_member(
                        _referenced_expression.value()->as<member_expression>()->get_name());
                    assert(member);

                    if (!member->is_constant())
                    {
                        return make_ready_future<expression *>(nullptr);
                    }

                    auto repl = replacements{};
                    return make_ready_future<expression *>(repl.claim(member).release());
                }

                assert(0);
            });
    }

    statement_ir postfix_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        auto base_expr_instructions = _base_expr->codegen_ir(ctx);

        if (!_modifier)
        {
            return base_expr_instructions;
        }

        auto base_variable_value = base_expr_instructions.back().result;
        auto base_variable = std::get<std::shared_ptr<codegen::ir::variable>>(base_variable_value);

        if (_modifier == lexer::token_type::dot)
        {
            auto access_instruction = codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::member_access_instruction>() },
                { base_variable, codegen::ir::label{ _accessed_member.value() } },
                { codegen::ir::make_variable(
                    _referenced_expression.value()->get_type()->codegen_type(ctx)) } };

            base_expr_instructions.push_back(std::move(access_instruction));

            return base_expr_instructions;
        }

        return _call_expression->codegen_ir(ctx);
    }

    constant_init_ir postfix_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }
}
}
