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
#include <vector>

#include <reaver/variant.h>

#include "../range.h"
#include "helpers.h"
#include "import_expression.h"
#include "literal.h"
#include "postfix_expression.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct lambda_expression;
    struct unary_expression;
    struct binary_expression;
    struct struct_literal;

    void print(const unary_expression & expr, std::ostream & os, std::size_t indent);
    void print(const binary_expression & expr, std::ostream & os, std::size_t indent);

    struct expression
    {
        range_type range;
        variant<literal<lexer::token_type::string>,
            literal<lexer::token_type::integer>,
            literal<lexer::token_type::boolean>,
            postfix_expression,
            import_expression,
            recursive_wrapper<lambda_expression>,
            recursive_wrapper<unary_expression>,
            recursive_wrapper<binary_expression>,
            recursive_wrapper<struct_literal>>
            expression_value = postfix_expression();
    };

    inline bool operator==(const expression & lhs, const expression & rhs)
    {
        return lhs.range == rhs.range && lhs.expression_value == rhs.expression_value;
    }

    expression parse_expression(context & ctx, expression_special_modes = expression_special_modes::none);

    void print(const expression & expr, std::ostream & os, std::size_t indent = 0);
}
}
