/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2015 Michał "Griwes" Dominiak
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

#include <boost/variant.hpp>

#include <reaver/visit.h>

#include "vapor/range.h"
#include "vapor/parser/helpers.h"
#include "vapor/parser/import_expression.h"
#include "vapor/parser/postfix_expression.h"
#include "vapor/parser/literal.h"

namespace reaver
{
    namespace vapor
    {
        namespace parser { inline namespace _v1
        {
            struct lambda_expression;
            struct unary_expression;
            struct binary_expression;

            void print(const unary_expression & expr, std::ostream & os, std::size_t indent);
            void print(const binary_expression & expr, std::ostream & os, std::size_t indent);

            struct expression
            {
                range_type range;
                boost::variant<
                    literal<lexer::token_type::string>,
                    literal<lexer::token_type::integer>,
                    postfix_expression,
                    import_expression,
                    boost::recursive_wrapper<lambda_expression>,
                    boost::recursive_wrapper<unary_expression>,
                    boost::recursive_wrapper<binary_expression>
                > expression_value;
            };

            expression parse_expression(context & ctx);

            void print(const expression & expr, std::ostream & os, std::size_t indent = 0);
        }}
    }
}
