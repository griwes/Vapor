/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/statements/statement.h"
#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/import.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/declaration.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/statements/if.h"
#include "vapor/analyzer/statements/return.h"
#include "vapor/parser.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<statement> preanalyze_statement(precontext & ctx,
        const parser::statement & parse,
        scope *& lex_scope)
    {
        return std::get<0>(fmap(parse.statement_value,
            make_overload_set(
                [&](const parser::declaration & decl) -> std::unique_ptr<statement> {
                    auto ret = preanalyze_declaration(ctx, decl, lex_scope);
                    return ret;
                },

                [&](const parser::return_expression & ret_expr) -> std::unique_ptr<statement> {
                    auto ret = preanalyze_return(ctx, ret_expr, lex_scope);
                    return ret;
                },

                [&](const parser::function_definition & func) -> std::unique_ptr<statement> {
                    auto ret = preanalyze_function_definition(ctx, func, lex_scope);
                    return ret;
                },

                [&](const parser::if_statement & if_stmt) -> std::unique_ptr<statement> {
                    auto ret = preanalyze_if_statement(ctx, if_stmt, lex_scope);
                    return ret;
                },

                [](auto &&) -> std::unique_ptr<statement> { assert(0); })));
    }
}
}
