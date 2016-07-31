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

#include "vapor/analyzer/statement.h"
#include "vapor/analyzer/declaration.h"
#include "vapor/analyzer/import.h"
#include "vapor/analyzer/function.h"

reaver::vapor::analyzer::_v1::statement reaver::vapor::analyzer::_v1::preanalyze_statement(const reaver::vapor::parser::_v1::statement & parse, reaver::vapor::analyzer::_v1::scope & lex_scope)
{
    return get<0>(fmap(parse.statement_value, make_overload_set(
        [&](const parser::declaration & decl) -> statement
        {
            return declaration(lex_scope, decl);
        },

        [](const parser::expression_list & expr_list) -> expression_list
        {
            assert(0);
        },

        [](const parser::function & func) -> function
        {
            assert(0);
        },

        [](auto &&) -> statement { assert(0); }
    )));
}

