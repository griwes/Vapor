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

#include "vapor/analyzer/types/member_assignment.h"
#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/expressions/member_assignment.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/unconstrained.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    member_assignment_type::~member_assignment_type() = default;

    future<std::vector<function *>> member_assignment_type::get_candidates(lexer::token_type tt) const
    {
        assert(!_assigned);

        if (tt != lexer::token_type::assign)
        {
            assert(0);
            return make_ready_future(std::vector<function *>{});
        }

        if (!_fun_storage)
        {
            auto overload = make_function("member assignment");
            overload->set_return_type(assigned_type()->get_expression());
            overload->set_parameters([&] {
                std::vector<std::unique_ptr<expression>> ret;
                ret.push_back(make_expression_ref(_expr, _expr->get_ast_info()));
                ret.push_back(make_runtime_value(builtin_types().unconstrained.get()));
                return ret;
            }());
            overload->add_analysis_hook([this](auto &&, auto && call_expr, std::vector<expression *> args) {
                assert(args.size() == 2);
                _expr->set_rhs(args.back());
                call_expr->replace_with(make_expression_ref(_expr, call_expr->get_ast_info()));

                return make_ready_future();
            });

            _fun_storage = std::move(overload);
        }

        return make_ready_future(std::vector<function *>{ _fun_storage->get() });
    }
}
}
