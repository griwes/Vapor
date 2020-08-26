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

#include "vapor/analyzer/types/typeclass.h"

#include <boost/algorithm/string/join.hpp>

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"

#include "expressions/type.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    typeclass_type::typeclass_type(std::vector<type *> param_types)
        : _param_types{ std::move(param_types) },
          _call_operator{ make_function("typeclass type call operator") }
    {
        auto call_operator_params =
            fmap(_param_types, [](auto && type) { return make_runtime_value(type, nullptr, std::nullopt); });
        call_operator_params.insert(
            call_operator_params.begin(), make_runtime_value(this, nullptr, std::nullopt));

        _call_operator->set_return_type(builtin_types().type->get_expression());
        _call_operator->set_parameters(std::move(call_operator_params));
        _call_operator->make_member();

        _call_operator->add_analysis_hook(
            [this](auto && ctx, call_expression * call_expr, std::vector<expression *> args) {
                assert(args.size() == 1 + _param_types.size());
                assert(args.front()->get_type() == this);

                auto tc_expr = args.front()->as<typeclass_expression>();
                auto tc = tc_expr->get_typeclass();
                args.erase(args.begin());

                return tc
                    ->type_for(ctx,
                        fmap(args,
                            [](auto && arg) {
                                assert(arg->is_constant());
                                auto type_expr = arg->template as<type_expression>();
                                assert(type_expr);
                                return type_expr->get_value();
                            }))
                    .then([call_expr](
                              auto && tc_type) { call_expr->replace_with(make_type_expression(tc_type)); });
            });
    }

    std::string typeclass_type::explain() const
    {
        return "typeclass ("
            + boost::join(fmap(_param_types, [](auto && param) { return param->explain(); }), ",") + ")";
    }

    void typeclass_type::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << explain() << styles::def << " @ " << styles::address
           << this << styles::def << ": builtin type\n";
    }

    future<std::vector<function *>> typeclass_type::get_candidates(lexer::token_type) const
    {
        return make_ready_future(std::vector<function *>{ _call_operator.get() });
    }

    std::unique_ptr<proto::type> typeclass_type::generate_interface() const
    {
        auto ret = std::make_unique<proto::type>();
        ret->set_allocated_reference(generate_interface_reference().release());
        return ret;
    }

    std::unique_ptr<proto::type_reference> typeclass_type::generate_interface_reference() const
    {
        auto ret = std::make_unique<proto::type_reference>();

        auto builtin_tc_ref = std::make_unique<proto::typeclass_type>();
        for (auto && param : _param_types)
        {
            *builtin_tc_ref->add_parameters() = *param->generate_interface_reference();
        }

        ret->set_allocated_typeclass(builtin_tc_ref.release());
        return ret;
    }
}
}
