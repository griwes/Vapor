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

#include "vapor/parser/statement.h"
#include "vapor/parser/lambda_expression.h"

reaver::vapor::parser::_v1::statement reaver::vapor::parser::_v1::parse_statement(reaver::vapor::parser::_v1::context & ctx)
{
    statement ret;

    if (peek(ctx, lexer::token_type::function))
    {
        auto func = parse_function(ctx);
        ret.range = func.range;
        ret.statement_value = std::move(func);
    }

    else
    {
        if (peek(ctx, lexer::token_type::let))
        {
            ret.statement_value = parse_declaration(ctx);
        }

        else if (peek(ctx, lexer::token_type::return_))
        {
            ret.statement_value = parse_return_expression(ctx);
        }

        else
        {
            ret.statement_value = parse_expression_list(ctx);
        }

        auto end = expect(ctx, lexer::token_type::semicolon).range.end();
        fmap(ret.statement_value, [&](const auto & value) -> unit { ret.range = { value.range.start(), end }; return {}; });
    }

    return ret;
}

void reaver::vapor::parser::_v1::print(const reaver::vapor::parser::_v1::statement & stmt, std::ostream & os, std::size_t indent)
{
    auto in = std::string(indent, ' ');

    os << in << "`statement` at " << stmt.range << '\n';
    os << in << "{\n";
    fmap(stmt.statement_value, [&](const auto & value) -> unit { print(value, os, indent + 4); return {}; });
    os << in << "}\n";
}
