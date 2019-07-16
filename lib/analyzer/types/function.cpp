/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017, 2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/types/function.h"
#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/function.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    function_type::function_type(type * ret, std::vector<type *> params)
        : _return{ ret }, _parameters{ std::move(params) }
    {
        auto call_operator_params =
            fmap(_parameters, [&](auto && param_type) { return make_runtime_value(param_type); });
        call_operator_params.insert(call_operator_params.begin(), make_runtime_value(this));

        _call_operator = make_function("function type " + function_type::explain() + " call operator");
        _call_operator->set_return_type(_return->get_expression());
        _call_operator->set_parameters(std::move(call_operator_params));

        _call_operator->make_member();

        _call_operator->add_analysis_hook(
            [this](analysis_context & ctx, call_expression * expr, std::vector<expression *> args) {
                assert(args.size() >= 1);
                assert(args.front()->get_type() == this);
                auto fun_expr = args.front()->as<function_expression>();
                assert(fun_expr->is_constant());
                auto function = fun_expr->get_value();
                assert(function);

                auto replacement = make_call_expression(function, nullptr, std::move(args));
                auto repl_ptr = replacement.get();
                expr->replace_with(std::move(replacement));

                return repl_ptr->analyze(ctx);
            });
    }

    future<std::vector<function *>> function_type::get_candidates(lexer::token_type op) const
    {
        if (op != lexer::token_type::round_bracket_open)
        {
            return make_ready_future<std::vector<function *>>({});
        }

        return make_ready_future<std::vector<function *>>({ _call_operator.get() });
    }
}
}
