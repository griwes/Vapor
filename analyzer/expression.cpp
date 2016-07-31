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

#include "vapor/analyzer/expression.h"
#include "vapor/analyzer/block.h"
#include "vapor/analyzer/lambda.h"
#include "vapor/analyzer/literal.h"
#include "vapor/analyzer/import.h"

reaver::vapor::analyzer::_v1::expression reaver::vapor::analyzer::_v1::preanalyze_expression(const reaver::vapor::parser::_v1::expression & expr, const reaver::vapor::analyzer::_v1::scope & lex_scope)
{
    return fmap(expr.expression_value, make_overload_set(
        [](const parser::string_literal & string)
        {
            assert(0);
            return literal();
        },

        [](const parser::integer_literal & integer)
        {
            assert(0);
            return literal();
        },

        [](const parser::postfix_expression & postfix)
        {
            assert(0);
            return postfix_expression();
        },

        [](const parser::import_expression & import)
        {
            assert(0);
            return import_expression();
        },

        [&](const parser::lambda_expression & lambda_expr)
        {
            return lambda(lambda_expr, lex_scope);
        },

        [](const parser::unary_expression & unary_expr)
        {
            assert(0);
            return unary_expression();
        },

        [](const parser::binary_expression & binary_expr)
        {
            assert(0);
            return binary_expression();
        }
    ));
}

reaver::vapor::analyzer::_v1::expression_list reaver::vapor::analyzer::_v1::preanalyze_expression_list(const reaver::vapor::parser::_v1::expression_list & expr, const reaver::vapor::analyzer::_v1::scope & lex_scope)
{
    return fmap(expr.expressions, [&](auto && expr){ return preanalyze_expression(expr, lex_scope); });
}

void reaver::vapor::analyzer::_v1::analyze(reaver::vapor::analyzer::_v1::expression & expr)
{
    fmap(expr, [](auto && v) { v.analyze(); return unit{}; });
}

reaver::vapor::analyzer::_v1::variable & reaver::vapor::analyzer::_v1::type_of(const reaver::vapor::analyzer::_v1::expression & expr)
{
    return get<0>(fmap(expr, [](auto && v) -> decltype(auto) { return type_of(v); }));
}

reaver::vapor::analyzer::_v1::variable & reaver::vapor::analyzer::_v1::value_of(const reaver::vapor::analyzer::_v1::expression & expr)
{
    return get<0>(fmap(expr, [](auto && v) -> decltype(auto) { return value_of(v); }));
}

