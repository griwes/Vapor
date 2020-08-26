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

#include "vapor/analyzer/expressions/entity.h"
#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/overload_set.h"
#include "vapor/analyzer/types/unresolved.h"
#include "vapor/codegen/ir/type.h"

#include "types/overload_set.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    future<std::vector<function *>> overload_set_type::get_candidates(lexer::token_type bracket) const
    {
        if (bracket == lexer::token_type::round_bracket_open)
        {
            return make_ready_future([&] { return _oset->get_overloads(); }());
        }

        assert(0);
        return make_ready_future(std::vector<function *>{});
    }

    void overload_set_type::_codegen_user_type(ir_generation_context & ctx,
        std::shared_ptr<codegen::ir::user_type> actual_type) const
    {
        auto overloads = _oset->get_overloads();
        bool is_vtable =
            std::any_of(overloads.begin(), overloads.end(), [](auto && fn) { return fn->vtable_slot(); });

        auto members = fmap(overloads, [&](function * fn) {
            if (is_vtable)
            {
                // vtable, which means no actual functions need to be generated; instead, a vtable layout must
                // be created

                // TODO: figure out a split somewhere that'd make this be dispatched on, instead of having a
                // branch completely changing the nature of the output language type that is being generated
                // here

                auto ret_expr = fn->return_type_expression();
                assert(ret_expr);
                auto ret_type_expr = ret_expr->as<type_expression>();
                assert(ret_type_expr);

                return codegen::ir::member{ codegen::ir::member_variable{ {},
                    codegen::ir::builtin_types().function(ret_type_expr->get_value()->codegen_type(ctx),
                        fmap(fn->parameters(),
                            [&](auto && param) { return param->get_type()->codegen_type(ctx); })) } };
            }

            // not vtable, which means this is a real overload set, which means it only needs to generate
            // function bodies for all the contained functions

            ctx.add_generated_function(fn);
            return codegen::ir::member{ fn->codegen_ir(ctx) };
        });
        auto type = codegen::ir::user_type{ get_scope()->get_entity_name(), 0, std::move(members) };

        fmap(type.members, [&](auto && member) {
            fmap(member,
                make_overload_set(
                    [&](codegen::ir::function & fn) {
                        fn.parent_type = actual_type;
                        fn.is_exported = _is_exported;

                        return unit{};
                    },
                    [&](codegen::ir::member_variable &) { return unit{}; },
                    [&](auto &&) {
                        assert(0);
                        return unit{};
                    }));
            return unit{};
        });

        *actual_type = std::move(type);
    }

    void overload_set_type::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << "overload set type";
        os << styles::def << " @ " << styles::address << this << styles::def << ": " << styles::string_value
           << utf8(get_scope()->get_name()) << '\n';
    }

    std::unique_ptr<google::protobuf::Message> overload_set_type::_user_defined_interface() const
    {
        auto t = std::make_unique<proto::overload_set_type>();

        for (auto && func : _oset->get_overloads())
        {
            auto fn = t->add_functions();
            fn->set_allocated_return_type(func->get_return_type()
                                              .try_get()
                                              .value()
                                              ->as<type_expression>()
                                              ->get_value()
                                              ->generate_interface_reference()
                                              .release());

            for (auto && param : func->parameters())
            {
                auto & par = *fn->add_parameters();
                par.set_allocated_type(param->get_type()->generate_interface_reference().release());
            }

            fn->set_is_member(func->is_member());

            if (auto slot = func->vtable_slot())
            {
                fn->set_has_vtable_id(true);
                fn->set_vtable_id(slot.value());
            }
        }

        return t;
    }
}
}
