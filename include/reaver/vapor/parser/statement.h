/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2016 Michał "Griwes" Dominiak
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
#include "expression_list.h"
#include "function.h"
#include "helpers.h"
#include "if_statement.h"
#include "return_expression.h"
#include "unary_expression.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct statement
    {
        range_type range;
        variant<declaration, return_expression, expression_list, function, if_statement> statement_value = expression_list();
    };

    inline bool operator==(const statement & lhs, const statement & rhs)
    {
        return lhs.range == rhs.range && lhs.statement_value == rhs.statement_value;
    }

    statement parse_statement(context & ctx);

    void print(const statement & stmt, std::ostream & os, std::size_t indent = 0);
}
}
