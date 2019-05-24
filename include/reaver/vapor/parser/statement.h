/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2019 Michał "Griwes" Dominiak
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

#include <reaver/variant.h>

#include "binary_expression.h"
#include "declaration.h"
#include "function.h"
#include "helpers.h"
#include "if_statement.h"
#include "return_expression.h"
#include "typeclass.h"
#include "unary_expression.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    enum class statement_mode
    {
        module,
        default_
    };

    struct statement
    {
        range_type range;
        std::variant<declaration,
            default_instance_definition,
            expression,
            return_expression,
            function_definition,
            if_statement>
            statement_value = declaration();
    };

    bool operator==(const statement & lhs, const statement & rhs);

    statement parse_statement(context & ctx, statement_mode mode = statement_mode::default_);

    void print(const statement & stmt, std::ostream & os, print_context ctx);
}
}
