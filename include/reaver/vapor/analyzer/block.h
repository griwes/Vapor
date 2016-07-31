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

#include <reaver/optional.h>

#include "vapor/parser/block.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/scope.h"
#include "vapor/analyzer/expression.h"
#include "vapor/analyzer/statement.h"
#include "vapor/analyzer/declaration.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class block
            {
            public:
                block(const parser::block & parse, const scope & lex_scope);

                block(const block &) = default;
                block(block &&) = default;

                void analyze()
                {
                    assert(0);
                }

            private:
                const parser::block & _parse;
                scope _block_scope;
                std::vector<variant<block, statement>> _value;
                std::vector<expression> _returned_expressions;
                optional<expression> _value_expression;
            };
        }}
    }
}

