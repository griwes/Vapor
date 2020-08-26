/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include "vapor/parser/parameter_list.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/instance_context.h"
#include "vapor/analyzer/semantic/parameter_list.h"
#include "vapor/analyzer/types/archetype.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/parameter_list.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    parameter_list preanalyze_parameter_list(precontext & prectx,
        const parser::parameter_list & param_list,
        scope * lex_scope,
        std::optional<instance_function_context> ctx)
    {
        std::size_t i = 0;
        replacements repl;

        return fmap(param_list.parameters, [&](auto && param_parse) {
            assert(param_parse.type || ctx);

            auto type = [&] {
                if (param_parse.type)
                {
                    return preanalyze_expression(prectx, param_parse.type.value(), lex_scope);
                }

                else if (ctx)
                {
                    return repl.claim(
                        ctx->original_overload->parameters()[i]->as<parameter>()->get_type_expression());
                }

                assert(!"a type not provided outside of an instance context");
            }();

            auto param = std::make_unique<parameter>(
                make_node(param_parse), lex_scope, param_parse.name.value.string, std::move(type));

            auto symb = make_symbol(param_parse.name.value.string, param.get());
            lex_scope->init(param_parse.name.value.string, std::move(symb));

            ++i;
            return param;
        });
    }

    parameter::parameter(ast_node parse,
        scope * lex_scope,
        std::u32string name,
        std::unique_ptr<expression> type)
        : expression{ lex_scope, std::move(name) }, _type_expression{ std::move(type) }
    {
        _set_ast_info(parse);
    }

    parameter::~parameter() = default;

    void parameter::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "parameter";
        print_address_range(os, this);
        os << ' ' << styles::string_value << utf8(get_name().value()) << '\n';

        auto type_expr_ctx = ctx.make_branch(true);
        os << styles::def << type_expr_ctx << styles::subrule_name << "type expression:\n";
        _type_expression->print(os, type_expr_ctx.make_branch(true));
    }

    template<typename Self>
    auto parameter::_get_replacement_helper(Self && self)
    {
        using Ret = decltype(self._get_replacement());

        if (self._archetype)
        {
            return static_cast<Ret>(self._archetype->get_expression());
        }

        return static_cast<Ret>(&self);
    }

    expression * parameter::_get_replacement()
    {
        auto repl = _get_replacement_helper(*this);
        assert(repl);
        return repl;
    }

    const expression * parameter::_get_replacement() const
    {
        auto repl = _get_replacement_helper(*this);
        assert(repl);
        return repl;
    }

    future<> parameter::_analyze(analysis_context & ctx)
    {
        return _type_expression->analyze(ctx).then([&] {
            auto type_value = _type_expression->as<type_expression>();
            assert(type_value);

            auto type = type_value->get_value();
            if (type->is_meta())
            {
                _archetype = type->generate_archetype(get_ast_info().value(), get_name().value());
            }

            this->_set_type(type);
        });
    }

    future<expression *> parameter::_simplify_expr(recursive_context ctx)
    {
        return _type_expression->simplify_expr(ctx).then(
            [&ctx = ctx.proper, this](auto && simpl) -> expression * {
                replace_uptr(_type_expression, simpl, ctx);
                return this;
            });
    }

    std::unique_ptr<expression> parameter::_clone_expr(replacements & repl) const
    {
        auto new_type = repl.try_get_replacement(get_type());
        return std::make_unique<parameter>(get_ast_info().value(),
            get_scope(),
            get_name().value(),
            new_type ? repl.copy_claim(new_type->get_expression()) : repl.claim(_type_expression.get()));
    }

    statement_ir parameter::_codegen_ir(ir_generation_context & ctx) const
    {
        auto var = codegen::ir::make_variable(get_type()->codegen_type(ctx));
        var->parameter = true;
        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::materialization_instruction>() },
            {},
            { std::move(var) } } };
    }

    constant_init_ir parameter::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    std::unique_ptr<google::protobuf::Message> parameter::_generate_interface() const
    {
        assert(0);
    }
}
}
