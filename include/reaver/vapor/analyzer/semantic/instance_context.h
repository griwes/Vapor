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

#pragma once

#include <string>

#include "../expressions/expression.h"
#include "../expressions/type.h"
#include "../simplification/replacements.h"
#include "scope.h"
#include "symbol.h"
#include "typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function;

    struct instance_context
    {
        const typeclass * tc;
        const std::vector<type *> & arguments;

        auto get_replacements() const
        {
            replacements repl;

            auto && params = tc->get_parameter_expressions();

            assert(arguments.size() == params.size());

            for (std::size_t i = 0; i < arguments.size(); ++i)
            {
                repl.add_replacement(params[i], arguments[i]->get_expression());
                repl.add_replacement(params[i]->_get_replacement(), arguments[i]->get_expression());

                auto type_param = params[i]->as<type_expression>();
                assert(type_param);
                repl.add_replacement(type_param->get_value(), arguments[i]);
            }

            return repl;
        };
    };

    struct instance_function_context
    {
        const function * original_overload;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    class function_definition;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    using function_definition_handler =
        reaver::unique_function<void(precontext & ctx, const parser::function_definition &)>;
}
}
