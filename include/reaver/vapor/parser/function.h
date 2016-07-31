/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015 Michał "Griwes" Dominiak
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

#include <reaver/variant.h>
#include <reaver/optional.h>

#include "vapor/range.h"
#include "vapor/parser/helpers.h"
#include "vapor/parser/argument_list.h"
#include "vapor/parser/expression.h"
#include "vapor/parser/lambda_expression.h"

namespace reaver
{
    namespace vapor
    {
        namespace parser { inline namespace _v1
        {
            struct block;

            struct function
            {
                range_type range;
                lexer::token name;
                optional<argument_list> arguments;
                optional<expression> return_type;
                recursive_wrapper<block> body;
            };

            function parse_function(context & ctx);

            void print(const function & f, std::ostream & os, std::size_t indent = 0);
        }}
    }
}

