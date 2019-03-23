/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019 Michał "Griwes" Dominiak
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

#include <reaver/future_get.h>

#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_instance> make_typeclass_instance(precontext & ctx,
        const parser::instance_literal & parse,
        scope * lex_scope)
    {
        auto scope = lex_scope->clone_for_class();
        auto scope_ptr = scope.get();

        return std::make_unique<typeclass_instance>(make_node(parse),
            std::move(scope),
            fmap(parse.typeclass_name.id_expression_value, [&](auto && t) { return t.value.string; }),
            fmap(parse.arguments.expressions,
                [&](auto && arg) { return preanalyze_expression(ctx, arg, scope_ptr); }));
    }

    typeclass_instance::typeclass_instance(ast_node parse,
        std::unique_ptr<scope> member_scope,
        std::vector<std::u32string> typeclass_name,
        std::vector<std::unique_ptr<expression>> arguments)
        : _node{ parse },
          _scope{ std::move(member_scope) },
          _typeclass_name{ std::move(typeclass_name) },
          _arguments{ std::move(arguments) }
    {
    }

    std::vector<expression *> typeclass_instance::get_arguments() const
    {
        return fmap(_arguments, [](auto && arg) { return arg.get(); });
    }

    future<> typeclass_instance::simplify_arguments(analysis_context & ctx)
    {
        return when_all(fmap(_arguments, [&](auto && arg) { return simplification_loop(ctx, arg); }))
            .then([](auto &&) {});
    }

    std::vector<function_definition *> typeclass_instance::get_member_function_defs() const
    {
        return fmap(_member_function_definitions, [](auto && def) { return def.get(); });
    }

    void typeclass_instance::set_type(typeclass_instance_type * type)
    {
        for (auto && [oset_name, oset] : type->get_overload_sets())
        {
            _member_overload_set_exprs.push_back(create_refined_overload_set(_scope.get(), oset_name, oset));
        }

        // close here, because if the delayed preanalysis later *adds* new members, then we have a bug... the
        // assertion that checks for closeness of the scope needs to be somehow weakened here, to allow for
        // more sensible error reporting than `assert`
        _scope->close();

        _type = type;
    }

    function_definition_handler typeclass_instance::get_function_definition_handler()
    {
        return [&](precontext & ctx, const parser::function_definition & parse) {
            auto scope = get_scope();
            auto func = preanalyze_function_definition(ctx, parse, scope, _type);
            assert(scope == get_scope());
            _member_function_definitions.push_back(std::move(func));
        };
    }

    void typeclass_instance::import_default_definitions(analysis_context & ctx)
    {
        replacements repl;
        std::unordered_map<function *, block *> function_block_defs;

        for (auto && roset_expr : _member_overload_set_exprs)
        {
            auto && roset = roset_expr->get_overload_set();
            roset->resolve_overrides();

            auto && base = roset->get_base();
            for (auto && fn : base->get_overloads())
            {
                assert(fn->vtable_slot());
                if (roset->get_vtable_entry(fn->vtable_slot().value()))
                {
                    continue;
                }

                assert(fn->get_body());

                _function_specialization fn_spec;
                fn_spec.spec = make_function(fn->get_explanation(), fn->get_range());
                repl.add_replacement(fn, fn_spec.spec.get());
                fn_spec.spec->set_return_type(fn->return_type_expression());
                fn_spec.spec->set_parameters(fn->parameters());

                function_block_defs.emplace(fn_spec.spec.get(), fn->get_body());

                roset->add_function(fn_spec.spec.get());

                _function_specializations.push_back(std::move(fn_spec));
            }

            roset->resolve_overrides();
        }

        for (auto && fn_spec : _function_specializations)
        {
            auto it = function_block_defs.find(fn_spec.spec.get());
            if (it == function_block_defs.end())
            {
                continue;
            }

            auto body_stmt = repl.copy_claim(it->second);

            auto body_block = dynamic_cast<block *>(body_stmt.get());
            assert(body_block);
            fn_spec.function_body.reset(body_block);
            body_stmt.release();

            fn_spec.spec->set_body(fn_spec.function_body.get());
        }
    }

    const typeclass * typeclass_instance::get_typeclass() const
    {
        return _type->get_typeclass();
    }

    void typeclass_instance::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << "typeclass instance";
        print_address_range(os, this);
        os << '\n';
    }
}
}
