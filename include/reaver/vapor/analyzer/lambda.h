/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014 Michał "Griwes" Dominiak
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

#include <vector>
#include <memory>

#include "vapor/range.h"
#include "vapor/operators.h"
#include "vapor/analyzer/type.h"
#include "vapor/analyzer/scope.h"
#include "vapor/analyzer/expression.h"
#include "vapor/analyzer/block.h"
#include "vapor/analyzer/function.h"
#include "vapor/analyzer/overloads.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class declaration;

            inline auto lambda(const parser::lambda_expression & parse, const scope & lex_scope)
            {
                assert(!parse.captures);
                assert(!parse.arguments);

                assert(0);

                auto type = analyzer::variable(types().type, make_type());
                type.range = parse.range;
                register_overload(type, operators::call, function(parse, {}, block(parse.body.get(), lex_scope)));

                return type;
            }
        }}
    }
}
