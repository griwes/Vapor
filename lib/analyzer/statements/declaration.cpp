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

#include "vapor/analyzer/statements/declaration.h"
#include "vapor/parser/declaration.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<declaration> _preanalyze_declaration(precontext & ctx,
        const parser::declaration & parse,
        scope * old_scope,
        scope * new_scope,
        declaration_type type)
    {
        switch (type)
        {
            case declaration_type::variable:
                assert(parse.rhs);
                break;

            case declaration_type::member:
                assert(parse.type_expression || parse.rhs);
                break;
        }

        auto ret = std::make_unique<declaration>(make_node(parse),
            parse.identifier.value.string,
            fmap(parse.rhs,
                [&](auto && expr) {
                    return preanalyze_expression(ctx, expr, old_scope, parse.identifier.value.string);
                }),
            fmap(parse.type_expression,
                [&](auto && expr) { return preanalyze_expression(ctx, expr, old_scope); }),
            new_scope,
            type);

        if (old_scope != new_scope)
        {
            new_scope->close();
        }

        if (parse.export_)
        {
            ret->mark_exported();
            ret->declared_symbol()
                ->get_expression_future()
                .then([](auto && expr) { expr->mark_exported(); })
                .detach();
        }

        return ret;
    }

    std::unique_ptr<declaration> preanalyze_declaration(precontext & ctx,
        const parser::declaration & parse,
        scope *& lex_scope)
    {
        auto old_scope = lex_scope;
        lex_scope = old_scope->clone_for_decl();
        return _preanalyze_declaration(ctx, parse, old_scope, lex_scope, declaration_type::variable);
    }

    std::unique_ptr<declaration> preanalyze_member_declaration(precontext & ctx,
        const parser::declaration & parse,
        scope * lex_scope)
    {
        return _preanalyze_declaration(ctx, parse, lex_scope, lex_scope, declaration_type::member);
    }

    declaration::declaration(ast_node parse,
        std::u32string name,
        std::optional<std::unique_ptr<expression>> init_expr,
        std::optional<std::unique_ptr<expression>> type_specifier,
        scope * scope,
        declaration_type decl_type)
        : _name{ std::move(name) },
          _type_specifier{ std::move(type_specifier) },
          _init_expr{ std::move(init_expr) },
          _type{ decl_type }
    {
        _set_ast_info(parse);

        auto symbol = make_symbol(_name);
        _declared_symbol = symbol.get();
        if (!scope->init(_name, std::move(symbol)))
        {
            assert(0);
        }
    }

    void declaration::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "declaration";
        print_address_range(os, this);
        os << ' ' << styles::string_value << utf8(_name) << '\n';

        auto type_ctx = ctx.make_branch(!_init_expr);
        os << styles::def << type_ctx << styles::subrule_name << "type of symbol:\n";
        _declared_symbol->get_type()->print(os, type_ctx.make_branch(true));

        if (_init_expr)
        {
            auto init_expr_ctx = ctx.make_branch(true);
            os << styles::def << init_expr_ctx << styles::subrule_name << "initializer expression:\n";
            _init_expr.value()->print(os, init_expr_ctx.make_branch(true));
        }
    }

    future<> declaration::_analyze(analysis_context & ctx)
    {
        auto fut = make_ready_future();

        fmap(_type_specifier, [&](auto && expr) {
            fut = fut.then([&]() { return expr->analyze(ctx); }).then([&]() {
                auto && type_expr = _type_specifier.value();
                assert(type_expr->get_type() == builtin_types().type);
                assert(type_expr->is_constant());
            });

            return unit{};
        });

        fmap(_init_expr, [&](auto && expr) {
            fut = fut.then([&]() { return expr->analyze(ctx); });

            fmap(_type_specifier, [&](auto && expr) {
                fut = fut.then([&]() {
                    auto type_var = expr->template as<type_expression>();
                    assert(_init_expr.value()->get_type() == type_var->get_value());
                });

                return unit{};
            });

            fut = fut.then([&] {
                auto expression = _init_expr.value().get();

                if (_type == declaration_type::member)
                {
                    _declared_member = make_member_expression(nullptr, _name, _init_expr.value()->get_type());
                    _declared_member.value()->set_default_value(_init_expr.value().get());
                    expression = _declared_member.value().get();
                }

                _declared_symbol->set_expression(expression);
            });

            return unit{};
        });

        if (!_init_expr)
        {
            fut = fut.then([&]() {
                assert(_type_specifier);
                auto type = _type_specifier.value()->as<type_expression>()->get_value();
                _declared_member = make_member_expression(nullptr, _name, type);

                _declared_symbol->set_expression(_declared_member.value().get());
            });
        }

        return fut;
    }

    std::unique_ptr<statement> declaration::_clone(replacements & repl) const
    {
        assert(_type == declaration_type::variable);
        return repl.claim(_init_expr.value().get());
    }

    future<statement *> declaration::_simplify(recursive_context ctx)
    {
        auto fut = make_ready_future<statement *>(this);

        fmap(_init_expr, [&](auto && expr) {
            fut = expr->simplify_expr(ctx)
                      .then([&, ctx](auto && simplified) {
                          replace_uptr(_init_expr.value(), simplified, ctx.proper);
                          return _declared_symbol->simplify(ctx);
                      })
                      .then([&]() -> statement * { return _init_expr->release(); });

            return unit{};
        });

        return fut;
    }

    statement_ir declaration::_codegen_ir(ir_generation_context & ctx) const
    {
        if (_declared_symbol->get_type() == builtin_types().type)
        {
            return {};
        }

        assert(_type == declaration_type::variable);
        auto ir = _init_expr.value()->codegen_ir(ctx);

        if (ir.back().result.index() == 0)
        {
            auto var = std::get<std::shared_ptr<codegen::ir::variable>>(ir.back().result);
            ir.back().declared_variable = var;
            var->name = _name;
        }
        return ir;
    }
}
}
