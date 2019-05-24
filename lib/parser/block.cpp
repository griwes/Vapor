/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015-2017, 2019 Michał "Griwes" Dominiak
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
    bool operator==(const block & lhs, const block & rhs)
    {
        return lhs.range == rhs.range && lhs.block_value == rhs.block_value
            && lhs.value_expression == rhs.value_expression;
    }

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
                ret.value_expression = parse_expression(ctx);
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
        ret.value_expression = std::move(expr);

        return ret;
    }

    void print(const block & bl, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "block";
        print_address_range(os, bl);
        os << '\n';

        auto statements_ctx = ctx.make_branch(!bl.value_expression);
        os << statements_ctx << styles::subrule_name << "statements:\n";

        std::size_t idx = 0;
        for (auto && element : bl.block_value)
        {
            fmap(element, [&](const auto & value) -> unit {
                print(value, os, statements_ctx.make_branch(++idx == bl.block_value.size()));
                return {};
            });
        }

        if (bl.value_expression)
        {
            auto value_ctx = ctx.make_branch(true);
            os << styles::def << value_ctx << styles::subrule_name << "value-expression:\n";

            print(*bl.value_expression, os, value_ctx.make_branch(true));
        }
    }
}
}
