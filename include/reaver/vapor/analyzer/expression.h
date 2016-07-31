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

#include <memory>

#include <reaver/prelude/functor.h>

#include "vapor/parser/expression_list.h"
#include "vapor/parser/expression.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/variable.h"
#include "vapor/analyzer/import.h"
#include "vapor/analyzer/literal.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class import_expression;

            class scope;

            class postfix_expression
            {
            public:
                void analyze()
                {
                    assert(0);
                }
            };

            class unary_expression
            {
            public:
                void analyze()
                {
                    assert(0);
                }
            };

            class binary_expression
            {
            public:
                void analyze()
                {
                    assert(0);
                }
            };

            using expression = variant<
                literal,
                variable,
                import_expression,
                postfix_expression,
                unary_expression,
                binary_expression
            >;

            using expression_list = std::vector<expression>;

            expression preanalyze_expression(const parser::expression & expr, const scope & lex_scope);
            expression_list preanalyze_expression_list(const parser::expression_list & expr, const scope & lex_scope);
            void analyze(expression & expr);
            variable & type_of(const expression &);
            variable & value_of(const expression &);
        }}
    }
}
