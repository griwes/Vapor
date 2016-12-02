/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015-2016 Michał "Griwes" Dominiak
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

#include "vapor/parser/block.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    block parse_block(context & ctx)
    {
        block ret;

        auto start = expect(ctx, lexer::token_type::curly_bracket_open).range.start();

        while (!peek(ctx, lexer::token_type::curly_bracket_close))
        {
            if (peek(ctx, lexer::token_type::curly_bracket_open))
            {
                ret.block_value.push_back(parse_block(ctx));
            }

            else if (peek(ctx, lexer::token_type::block_value))
            {
                expect(ctx, lexer::token_type::block_value);
                ret.value_expression = parse_expression_list(ctx);
                break;
            }

            else
            {
                ret.block_value.push_back(parse_statement(ctx));
            }
        }

        auto end = expect(ctx, lexer::token_type::curly_bracket_close).range.end();

        ret.range = { start, end };

        return ret;
    }

    block parse_single_statement_block(context & ctx)
    {
        block ret;

        auto start = expect(ctx, lexer::token_type::block_value).range.start();
        auto expr = parse_expression(ctx);
        ret.range = { start, expr.range.end() };
        ret.value_expression = expression_list{ expr.range, { std::move(expr) } };

        return ret;
    }

    void print(const block & bl, std::ostream & os, std::size_t indent)
    {
        auto in = std::string(indent, ' ');

        os << in << "`block` at " << bl.range << '\n';

        os << in << "{\n";
        {
            auto in = std::string(indent + 4, ' ');
            for (auto && element : bl.block_value)
            {
                os << in << "{\n";
                visit(
                    [&](const auto & value) -> unit {
                        print(value, os, indent + 8);
                        return {};
                    },
                    element);
                os << in << "}\n";
            }

            if (bl.value_expression)
            {
                os << in << "value_expression:\n";
                os << in << "{\n";
                print(*bl.value_expression, os, indent + 8);
                os << in << "}\n";
            }
        }

        os << in << "}\n";
    }
}
}
