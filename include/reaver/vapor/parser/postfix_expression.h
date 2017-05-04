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

#include <reaver/optional.h>
#include "../range.h"
#include "helpers.h"
#include "literal.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct expression;
    struct expression_list;

    struct postfix_expression
    {
        range_type range;
        variant<identifier, recursive_wrapper<expression_list>> base_expression = identifier();
        optional<lexer::token_type> modifier_type = none;
        std::vector<expression> arguments = {};
        optional<identifier> accessed_member = none;
    };

    inline bool operator==(const postfix_expression & lhs, const postfix_expression & rhs)
    {
        return lhs.range == rhs.range && lhs.base_expression == rhs.base_expression && lhs.modifier_type == rhs.modifier_type && lhs.arguments == rhs.arguments
            && lhs.accessed_member == rhs.accessed_member;
    }

    postfix_expression parse_postfix_expression(context & ctx, expression_special_modes = expression_special_modes::none);

    void print(const postfix_expression & expr, std::ostream & os, std::size_t indent = 0);
}
}
