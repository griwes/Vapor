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

#include "vapor/parser/helpers.h"
#include "vapor/parser/postfix_expression.h"
#include "vapor/parser/expression_list.h"
#include "vapor/parser/lambda_expression.h"

reaver::vapor::parser::_v1::postfix_expression reaver::vapor::parser::_v1::parse_postfix_expression(reaver::vapor::parser::context & ctx)
{
    auto closing = [](lexer::token_type type)
    {
        using namespace lexer;
        switch (type)
        {
            case token_type::angle_bracket_open: return token_type::angle_bracket_close;
            case token_type::round_bracket_open: return token_type::round_bracket_close;
            case token_type::square_bracket_open: return token_type::square_bracket_close;
            case token_type::curly_bracket_open: return token_type::curly_bracket_close;
            default: throw exception(logger::crash) << "invalid opening bracket type";
        }
    };

    postfix_expression ret;

    position start, end;

    if (peek(ctx, lexer::token_type::round_bracket_open))
    {
        start = expect(ctx, lexer::token_type::round_bracket_open).range.start();
        ret.base_expression = parse_expression_list(ctx);
        end = expect(ctx, lexer::token_type::round_bracket_close).range.end();;
    }

    else
    {
        ret.base_expression = parse_id_expression(ctx);
        auto & range = get<id_expression>(ret.base_expression).range;
        start = range.start();
        end = range.end();
    }

    for (auto && type : { lexer::token_type::round_bracket_open, lexer::token_type::square_bracket_open, lexer::token_type::curly_bracket_open,
        lexer::token_type::angle_bracket_open })
    {
        if (peek(ctx, type))
        {
            ret.bracket_type = type;
            expect(ctx, type);

            break;
        }
    }

    if (ret.bracket_type)
    {
        if (!peek(ctx, closing(*ret.bracket_type)))
        {
            ret.arguments.push_back(parse_expression(ctx));
            while (peek(ctx, lexer::token_type::comma))
            {
                expect(ctx, lexer::token_type::comma);
                ret.arguments.push_back(parse_expression(ctx));
            }
        }
        end = expect(ctx, closing(*ret.bracket_type)).range.end();
    }

    ret.range = { start, end };

    return ret;
}

void reaver::vapor::parser::_v1::print(const reaver::vapor::parser::_v1::postfix_expression & expr, std::ostream & os, std::size_t indent)
{
    auto in = std::string(indent, ' ');

    os << in << "`postfix-expression` at " << expr.range << '\n';
    if (expr.bracket_type)
    {
        os << in << "bracket type: `" << lexer::token_types[+*expr.bracket_type] << "`\n";
    }

    os << in << "{\n";

    fmap(expr.base_expression, [&](const auto & value) -> unit { print(value, os, indent + 4); return {}; });
    if (expr.arguments.size())
    {
        auto in = std::string(indent + 4, ' ');
        os << in << "{\n";
        fmap(expr.arguments, [&](auto && arg) -> unit { print(arg, os, indent + 8); return {}; });
        os << in << "{\n";
    }

    os << in << "}\n";
}

