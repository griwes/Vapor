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

#include <numeric>

#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/statements/block.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<function_declaration> preanalyze_function_declaration(precontext & ctx,
        const parser::function_declaration & parse,
        scope *& lex_scope)
    {
        auto function_scope = lex_scope->clone_for_local();

        parameter_list params;
        if (parse.parameters)
        {
            params = preanalyze_parameter_list(ctx, parse.parameters.value(), function_scope.get());
        }
        function_scope->close();
        auto function_scope_ptr = function_scope.get();

        return std::make_unique<function_declaration>(make_node(parse),
            parse.name.value.string,
            std::move(params),
            fmap(parse.return_type,
                [&](auto && ret_type) { return preanalyze_expression(ctx, ret_type, function_scope_ptr); }),
            std::move(function_scope));
    }

    std::unique_ptr<function_definition> preanalyze_function_definition(precontext & prectx,
        const parser::function_definition & parse,
        scope *& lex_scope,
        typeclass_instance_type * instance_type)
    {
        auto function_scope = lex_scope->clone_for_local();

        function * original_overload = nullptr;
        std::optional<instance_function_context> fn_ctx;
        if (instance_type)
        {
            auto oset = instance_type->get_overload_sets().at(parse.signature.name.value.string);
            auto && overloads = oset->get_overloads();

            auto pred = [&](auto && fn) {
                auto fn_params_size = fn->parameters().size();
                bool has_parameters = parse.signature.parameters.has_value();
                auto parse_params_size = has_parameters ? parse.signature.parameters->parameters.size() : 0;
                return fn_params_size == parse_params_size;
            };
            auto count = std::count_if(overloads.begin(), overloads.end(), pred);
            if (count != 1)
            {
                if (count == 0)
                {
                    assert(!"no matching typeclass function found");
                }

                else
                {
                    assert(!"overloaded function with the same arity can't yet be used to infer types");
                }
            }

            auto it = std::find_if(overloads.begin(), overloads.end(), pred);
            assert(it != overloads.end());
            original_overload = *it;

            fn_ctx.emplace(instance_function_context{ original_overload });
        }

        parameter_list params;
        if (parse.signature.parameters)
        {
            params = preanalyze_parameter_list(
                prectx, parse.signature.parameters.value(), function_scope.get(), fn_ctx);
        }
        function_scope->close();

        auto ret_type = fmap(parse.signature.return_type,
            [&](auto && ret_type) { return preanalyze_expression(prectx, ret_type, function_scope.get()); });
        if (!ret_type && instance_type)
        {
            auto original_ret = original_overload->return_type_expression();
            assert(original_ret);
            replacements repl;
            ret_type = repl.claim(original_ret);
        }

        auto ret = std::make_unique<function_definition>(make_node(parse),
            parse.signature.name.value.string,
            std::move(params),
            std::move(ret_type),
            preanalyze_block(prectx, *parse.body, function_scope.get(), true),
            std::move(function_scope));

        if (parse.signature.export_)
        {
            auto expr_symbol = lex_scope->get(parse.signature.name.value.string);
            expr_symbol->mark_exported();
            expr_symbol->get_expression()->mark_exported();
        }

        return ret;
    }

    function_declaration::function_declaration(ast_node parse,
        std::u32string name,
        parameter_list params,
        std::optional<std::unique_ptr<expression>> return_type,
        std::unique_ptr<scope> scope)
        : _scope{ std::move(scope) },
          _name{ std::move(name) },
          _parameter_list{ std::move(params) },
          _return_type{ std::move(return_type) },
          _overload_set_expression{ get_overload_set(_scope->parent(), _name) }
    {
        _set_ast_info(parse);

        _function = make_function(utf8(_name), get_ast_info().value().range);
        _function->set_name(U"call");
        _overload_set_expression->get_overload_set()->add_function(_function.get());
    }

    function_definition::function_definition(ast_node parse,
        std::u32string name,
        parameter_list params,
        std::optional<std::unique_ptr<expression>> return_type,
        std::unique_ptr<block> body,
        std::unique_ptr<scope> scope)
        : function_declaration{ parse,
              std::move(name),
              std::move(params),
              std::move(return_type),
              std::move(scope) },
          _body{ std::move(body) }
    {
    }

    void function_declaration::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "function-declaration";
        print_address_range(os, this);
        os << ' ' << styles::string_value << utf8(_name) << '\n';

        auto fn_ctx = ctx.make_branch(false);
        os << styles::def << fn_ctx << styles::rule_name << "declared function:\n";
        _function->print(os, fn_ctx.make_branch(true), true);

        if (_parameter_list.size())
        {
            auto params_ctx = ctx.make_branch(false);
            os << styles::def << params_ctx << styles::subrule_name << "parameters:\n";

            std::size_t idx = 0;
            for (auto && param : _parameter_list)
            {
                param->print(os, params_ctx.make_branch(++idx == _parameter_list.size()));
            }
        }

        auto return_type_ctx = ctx.make_branch(true);
        os << styles::def << return_type_ctx << styles::subrule_name << "return type:\n";
        _function->return_type_expression()->print(os, return_type_ctx.make_branch(true));
    }

    void function_definition::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "function-definition";
        print_address_range(os, this);
        os << ' ' << styles::string_value << utf8(_name) << '\n';

        function_declaration::print(os, ctx.make_branch(false));

        auto body_ctx = ctx.make_branch(true);
        os << styles::def << body_ctx << styles::subrule_name << "body:\n";
        _body->print(os, body_ctx.make_branch(true));
    }

    future<> function_declaration::_analyze(analysis_context & ctx)
    {
        auto initial_future = [&] {
            if (!_return_type)
            {
                return make_ready_future();
            }

            return (*_return_type)->analyze(ctx).then([&] {
                auto & type_expr = *_return_type;
                assert(type_expr->get_type() == builtin_types().type);
                assert(type_expr->is_constant());

                _function->set_return_type(_return_type->get()->_get_replacement());
            });
        }();

        return initial_future
            .then([&] {
                return when_all(fmap(_parameter_list, [&](auto && param) { return param->analyze(ctx); }));
            })
            .then([&] {
                _function->set_parameters(
                    fmap(_parameter_list, [](auto && param) -> expression * { return param.get(); }));
            });
    }

    future<> function_definition::_analyze(analysis_context & ctx)
    {
        auto base_future = function_declaration::_analyze(ctx);

        return base_future
            .then([&] {
                _function->set_name(
                    _overload_set_expression->get_overload_set()->get_scope()->get_entity_name() + U".call");
                _function->set_codegen([=](ir_generation_context & ctx) {
                    auto ret = codegen::ir::function{
                        fmap(_parameter_list,
                            [&](auto && param) {
                                return std::get<std::shared_ptr<codegen::ir::variable>>(
                                    param->codegen_ir(ctx).back().result);
                            }),
                        _body->codegen_return(ctx),
                        _body->codegen_ir(ctx)
                    };
                    return ret;
                });

                return _body->analyze(ctx);
            })
            .then([&] {
                fmap(_return_type, [&](auto && ret_type) {
                    auto type_expr = ret_type->template as<type_expression>();
                    assert(type_expr->get_value() == _body->return_type());

                    return unit{};
                });

                _function->set_body(_body.get());

                if (!_return_type)
                {
                    _function->set_return_type(make_type_expression(_body->return_type()));
                }
            });
    }

    future<statement *> function_declaration::_simplify(recursive_context ctx)
    {
        return make_ready_future<statement *>(this);
    }

    future<statement *> function_definition::_simplify(recursive_context ctx)
    {
        return _body->simplify(ctx).then([&](auto && simplified) -> statement * {
            replace_uptr(_body, dynamic_cast<block *>(simplified), ctx.proper);
            return this;
        });
    }

    statement_ir function_declaration::_codegen_ir(ir_generation_context &) const
    {
        return {};
    }

    statement_ir function_definition::_codegen_ir(ir_generation_context &) const
    {
        return {};
    }
}
}
