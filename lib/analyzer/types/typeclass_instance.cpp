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

#include "vapor/analyzer/types/typeclass_instance.h"

#include <boost/algorithm/string.hpp>

#include "vapor/analyzer/expressions/member.h"
#include "vapor/analyzer/semantic/typeclass.h"
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/block.h"
#include "vapor/analyzer/statements/function.h"

#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    typeclass_instance_type::typeclass_instance_type(typeclass * tc, std::vector<type *> arguments)
        : _arguments{ std::move(arguments) }, _ctx{ tc, _arguments }
    {
        auto repl = _ctx.get_replacements();
        std::unordered_map<function *, block *> function_block_defs;

        for (auto && symb : tc->get_scope()->symbols_in_order())
        {
            auto && oset = symb->get_expression()->as<overload_set_expression>();
            if (oset)
            {
                auto && name = symb->get_name();

                for (auto && fn : oset->get_overload_set()->get_overloads())
                {
                    _function_instance fn_instance;
                    fn_instance.instance = make_function(fn->get_explanation(), fn->get_range());
                    repl.add_replacement(fn, fn_instance.instance.get());
                    fn_instance.return_type_expression = repl.copy_claim(fn->return_type_expression());
                    fn_instance.instance->set_return_type(fn_instance.return_type_expression);
                    fn_instance.parameter_expressions =
                        fmap(fn->parameters(), [&](auto && param) { return repl.copy_claim(param); });
                    fn_instance.instance->set_parameters(
                        fmap(fn_instance.parameter_expressions, [](auto && expr) { return expr.get(); }));
                    fmap(fn->vtable_slot(), [&](auto && id) {
                        fn_instance.instance->mark_virtual(id);
                        return unit{};
                    });

                    if (fn->get_body())
                    {
                        function_block_defs.emplace(fn_instance.instance.get(), fn->get_body());
                    }

                    fn_instance.overload_set_expression = get_overload_set_special(_oset_scope.get(), name);
                    fn_instance.overload_set_expression->get_overload_set()->add_function(
                        fn_instance.instance.get());

                    _osets.emplace(name, fn_instance.overload_set_expression->get_overload_set());

                    _function_instances.push_back(std::move(fn_instance));
                }
            }
        }

        for (auto && fn_instance : _function_instances)
        {
            auto it = function_block_defs.find(fn_instance.instance.get());
            if (it == function_block_defs.end())
            {
                continue;
            }

            auto body_stmt = repl.copy_claim(it->second);

            auto body_block = dynamic_cast<block *>(body_stmt.get());
            assert(body_block);
            fn_instance.function_body.reset(body_block);
            body_stmt.release();

            fn_instance.instance->set_body(fn_instance.function_body.get());
        }

        _member_expressions.reserve(_osets.size());
        for (auto && [oset_name, oset] : _osets)
        {
            auto member_expression = make_member_expression(this, oset_name, oset->get_type());
            get_scope()->init(oset_name, make_symbol(oset_name, member_expression.get()));
            _member_expressions.push_back(std::move(member_expression));
        }
    }

    std::string typeclass_instance_type::explain() const
    {
        return "a typeclass instance type (TODO: add name tracking to this)";
    }

    void typeclass_instance_type::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << explain() << '\n';
    }

    std::unique_ptr<proto::type> typeclass_instance_type::generate_interface() const
    {
        assert(0);
    }

    std::unique_ptr<proto::type_reference> typeclass_instance_type::generate_interface_reference() const
    {
        auto ret = std::make_unique<proto::type_reference>();

        auto type = std::make_unique<proto::typeclass_instance_type>();
        type->set_allocated_typeclass(_ctx.tc->generate_interface_reference().release());
        for (auto && arg : _arguments)
        {
            *type->add_arguments() = *arg->generate_interface_reference();
        }

        ret->set_allocated_typeclass_instance_type(type.release());
        return ret;
    }

    future<std::unique_ptr<expression>> typeclass_instance_type::get_default_instance_expr(
        analysis_context & ctx) const
    {
        return ctx.default_instances_future().then([&]() -> std::unique_ptr<expression> { assert(0); });
    }

    future<> typeclass_instance_type::_analyze(analysis_context & ctx)
    {
        return when_all(fmap(_function_instances, [&](auto && instance) {
            return when_all(
                fmap(instance.parameter_expressions, [&](auto && expr) { return expr->analyze(ctx); }))
                .then([&] { return instance.return_type_expression->analyze(ctx); })
                .then([&] {
                    if (instance.function_body)
                    {
                        return instance.function_body->analyze(ctx);
                    }
                    return make_ready_future();
                });
        }));
    }

    void typeclass_instance_type::_codegen_type(ir_generation_context & ctx,
        std::shared_ptr<codegen::ir::user_type> actual_type) const
    {
        auto members = fmap(_member_expressions, [&](auto && member) {
            auto ret = member->member_codegen_ir(ctx);

            // set scopes on the overload set type
            auto udt = dynamic_cast<codegen::ir::user_type *>(ret.type.get());
            assert(udt); // ...a type of an overload set better be an UDT...
            udt->scopes = codegen_scopes(ctx);

            return codegen::ir::member{ std::move(ret) };
        });

        auto type =
            codegen::ir::user_type{ _codegen_name(ctx), get_scope()->codegen_ir(), 0, std::move(members) };

        *actual_type = std::move(type);
    }

    std::u32string typeclass_instance_type::_codegen_name(ir_generation_context & ctx) const
    {
        return U"tci$"
            + boost::join(
                fmap(_ctx.tc->get_scope()->codegen_ir(), [](auto && scope) { return scope.name; }), U".")
            + U"." + _ctx.tc->codegen_name(ctx) + U"("
            + boost::join(
                fmap(_arguments, [&](type * arg) { return arg->codegen_scopes(ctx).back().name; }), ", ")
            + U")";
    }
}
}
