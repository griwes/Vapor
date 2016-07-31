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
#include "vapor/parser/function.h"
#include "vapor/parser/lambda_expression.h"
#include "vapor/parser/block.h"

reaver::vapor::parser::_v1::function reaver::vapor::parser::_v1::parse_function(context & ctx)
{
    function ret;

    auto start = expect(ctx, lexer::token_type::function).range.start();

    ret.name = expect(ctx, lexer::token_type::identifier);
    expect(ctx, lexer::token_type::round_bracket_open);
    if (peek(ctx) && peek(ctx)->type != lexer::token_type::round_bracket_close)
    {
        ret.arguments = parse_argument_list(ctx);
    }
    expect(ctx, lexer::token_type::round_bracket_close);

    if (peek(ctx) && peek(ctx)->type == lexer::token_type::indirection)
    {
        ret.return_type = parse_expression(ctx);
    }

    ret.body = parse_block(ctx);

    ret.range = { start, ret.body->range.end() };

    return ret;
}

void reaver::vapor::parser::_v1::print(const reaver::vapor::parser::_v1::function & f, std::ostream & os, std::size_t indent)
{
    auto in = std::string(indent, ' ');

    assert(!f.arguments && !f.return_type);

    os << in << "`function` at " << f.range << '\n';
    os << in << "{\n";
    os << std::string(indent + 4, ' ') << f.name << '\n';
    print(*f.body, os, indent + 4);
    os << in << "}\n";
}

