/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/closure.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/closure.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<closure> preanalyze_closure(precontext & ctx,
        const parser::lambda_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        assert(!parse.captures);

        auto local_scope = lex_scope->clone_for_local();
        parameter_list params;

        if (parse.parameters)
        {
            params = preanalyze_parameter_list(ctx, parse.parameters.value(), local_scope.get());
        }
        local_scope->close();
        auto scope = local_scope.get();

        return std::make_unique<closure>(make_node(parse),
            std::move(local_scope),
            std::move(params),
            preanalyze_block(ctx, parse.body, scope, true),
            fmap(parse.return_type,
                [&](auto && ret_type) { return preanalyze_expression(ctx, ret_type, scope); }),
            lex_scope,
            std::move(name));
    }

    closure::closure(ast_node parse,
        std::unique_ptr<scope> sc,
        parameter_list params,
        std::unique_ptr<block> body,
        std::optional<std::unique_ptr<expression>> return_type,
        scope * lex_scope,
        std::optional<std::u32string> name)
        : expression{ lex_scope, std::move(name) },
          _parameter_list{ std::move(params) },
          _return_type{ std::move(return_type) },
          _scope{ std::move(sc) },
          _body{ std::move(body) }
    {
        _set_ast_info(parse);
    }

    void closure::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "closure";
        print_address_range(os, this);
        os << '\n';

        auto type_ctx = ctx.make_branch(false);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        get_type()->print(os, type_ctx.make_branch(true));

        if (_parameter_list.size())
        {
            auto param_ctx = ctx.make_branch(false);
            os << styles::def << param_ctx << styles::subrule_name << "parameters:\n";

            std::size_t idx = 0;
            for (auto && param : _parameter_list)
            {
                param->print(os, param_ctx.make_branch(++idx == _parameter_list.size()));
            }
        }

        auto return_type_ctx = ctx.make_branch(false);
        os << styles::def << return_type_ctx << styles::subrule_name << "return type:\n";
        _body->return_type()->print(os, return_type_ctx.make_branch(true));

        auto body_ctx = ctx.make_branch(true);
        os << styles::def << body_ctx << styles::subrule_name << "body:\n";
        _body->print(os, body_ctx.make_branch(true));
    }

    future<> closure::_analyze(analysis_context & ctx)
    {
        _scope->set_name(U"closure$" + utf32(std::to_string(ctx.closure_index++)));

        auto initial_future = [&] {
            if (_return_type)
            {
                return (*_return_type)->analyze(ctx).then([&] {
                    auto & type_expr = *_return_type;

                    assert(type_expr->get_type() == builtin_types().type);
                    assert(type_expr->is_constant());
                });
            }

            return make_ready_future();
        }();

        return initial_future
            .then([&] {
                return when_all(fmap(_parameter_list, [&](auto && param) { return param->analyze(ctx); }));
            })
            .then([&] { return _body->analyze(ctx); })
            .then([&] {
                fmap(_return_type, [&](auto && ret_type) {
                    auto type_expr = ret_type->template as<type_expression>();
                    assert(type_expr->get_value() == _body->return_type());

                    return unit{};
                });

                auto ret_expr = _return_type ? _return_type->get()->_get_replacement()
                                             : _body->return_type()->get_expression();

                auto function = make_function("closure call", get_ast_info().value().range);
                function->set_name(_scope->get_entity_name() + U".call");
                function->set_body(_body.get());
                function->set_return_type(ret_expr);
                function->set_parameters(
                    fmap(_parameter_list, [](auto && param) -> expression * { return param.get(); }));
                function->set_codegen([this](ir_generation_context & ctx) {
                    auto ret = codegen::ir::function{
                        fmap(_parameter_list,
                            [&](auto && param) {
                                return std::get<std::shared_ptr<codegen::ir::variable>>(
                                    param->codegen_ir(ctx).back().result);
                            }),
                        _body->codegen_return(ctx),
                        _body->codegen_ir(ctx),
                    };
                    ret.is_member = true;
                    return ret;
                });

                auto fn_ptr = function.get();
                _type = std::make_unique<closure_type>(this, std::move(_scope), std::move(function));
                this->_set_type(_type.get());
            });
    }

    std::unique_ptr<expression> closure::_clone_expr(replacements & repl) const
    {
        assert(!"this shouldn't be called, or, when called, should return an empty expression...");
    }

    future<expression *> closure::_simplify_expr(recursive_context ctx)
    {
        return _body->simplify(ctx).then([&](auto && simplified) -> expression * {
            replace_uptr(_body, dynamic_cast<block *>(simplified), ctx.proper);
            return this;
        });
    }

    statement_ir closure::_codegen_ir(ir_generation_context & ctx) const
    {
        // assert(0); // something is fucky in this function
        auto var = codegen::ir::make_variable(get_type()->codegen_type(ctx));
        if (get_name())
        {
            var->name =
                std::u32string{ get_type()->get_scope()->parent()->get_scoped_name() } + get_name().value();
        }
        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::materialization_instruction>() },
            {},
            { std::move(var) } } };
    }

    constant_init_ir closure::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    declaration_ir closure::declaration_codegen_ir(ir_generation_context & ctx) const
    {
        return { { std::get<std::shared_ptr<codegen::ir::variable>>(codegen_ir(ctx).back().result) } };
    }
}
}
