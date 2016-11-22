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
#include "argument_list.h"
#include "capture_list.h"
#include "block.h"

namespace reaver::vapor::parser { inline namespace _v1
{
    struct lambda_expression
    {
        range_type range;
        optional<capture_list> captures;
        optional<argument_list> arguments;
        optional<expression> return_type;
        block body;
    };

    inline bool operator==(const lambda_expression & lhs, const lambda_expression & rhs)
    {
        return lhs.range == rhs.range
            && lhs.captures == rhs.captures
            && lhs.arguments == rhs.arguments
            && lhs.return_type == rhs.return_type
            && lhs.body == rhs.body;
    }

    lambda_expression parse_lambda_expression(context & ctx);

    void print(const lambda_expression & expr, std::ostream & os, std::size_t indent = 0);
}}

