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

#include <vector>

#include <boost/variant.hpp>

#include "vapor/parser/function.h"
#include "vapor/parser/lambda_expression.h"
#include "vapor/analyzer/declaration.h"
#include "vapor/analyzer/block.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class function
            {
            public:
                function(reaver::variant<const parser::function &, const parser::lambda_expression &> parse, std::vector<declaration> arguments, block body) : _parse(parse)
                {
                }

            private:
                reaver::variant<const parser::function &, const parser::lambda_expression &> _parse;
            };
        }}
    }
}

