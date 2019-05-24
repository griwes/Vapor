/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2017, 2019 Michał "Griwes" Dominiak
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
#include "member_expression.h"
#include "postfix_expression.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct lambda_expression;
    struct unary_expression;
    struct binary_expression;
    struct struct_literal;
    struct typeclass_literal;
    struct instance_literal;
    struct default_instance_expression;

    void print(const unary_expression & expr, std::ostream & os, std::size_t indent);
    void print(const binary_expression & expr, std::ostream & os, std::size_t indent);

    struct expression
    {
        range_type range;
        std::variant<literal<lexer::token_type::string>,
            literal<lexer::token_type::integer>,
            literal<lexer::token_type::boolean>,
            member_expression, // this might need to be pulled into postfix expressions later on
                               // (if I want to make this work more like complex lenses)
            postfix_expression,
            import_expression,
            recursive_wrapper<lambda_expression>,
            recursive_wrapper<unary_expression>,
            recursive_wrapper<binary_expression>,
            recursive_wrapper<struct_literal>,
            recursive_wrapper<typeclass_literal>,
            recursive_wrapper<instance_literal>>
            expression_value = postfix_expression();
    };

    bool operator==(const expression & lhs, const expression & rhs);

    expression parse_expression(context & ctx, expression_special_modes = expression_special_modes::none);

    void print(const expression & expr, std::ostream & os, print_context ctx);
}
}
