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

#include <reaver/variant.h>

#include "vapor/range.h"
#include "vapor/parser/helpers.h"
#include "vapor/parser/id_expression.h"
#include "vapor/parser/literal.h"

namespace reaver
{
    namespace vapor
    {
        namespace parser { inline namespace _v1
        {
            struct import_expression
            {
                range_type range;
                reaver::variant<id_expression, string_literal> module_name = id_expression();
            };

            import_expression parse_import_expression(context & ctx);

            void print(const import_expression & expr, std::ostream & os, std::size_t indent = 0);
        }}
    }
}
