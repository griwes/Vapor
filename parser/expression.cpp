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

#include "vapor/parser/expression.h"
#include "vapor/parser/lambda_expression.h"

reaver::vapor::parser::_v1::expression reaver::vapor::parser::_v1::parse_expression(reaver::vapor::parser::_v1::context & ctx, bool special_assignment)
{
    expression ret;
    auto type = peek(ctx)->type;

    if (peek(ctx, lexer::token_type::string))
    {
        ret.expression_value = parse_literal<lexer::token_type::string>(ctx);
    }

    else if (peek(ctx, lexer::token_type::integer))
    {
        ret.expression_value = parse_literal<lexer::token_type::integer>(ctx);
    }

    else if (peek(ctx, lexer::token_type::identifier))
    {
        ret.expression_value = parse_postfix_expression(ctx);
    }

    else if (peek(ctx, lexer::token_type::import))
    {
        ret.expression_value = parse_import_expression(ctx);
    }

    else if (peek(ctx, lexer::token_type::lambda))
    {
        ret.expression_value = parse_lambda_expression(ctx);
    }

    else if (is_unary_operator(type))
    {
        ret.expression_value = parse_unary_expression(ctx);
    }

    else
    {
        throw expectation_failure{ "expression", ctx.begin->string, ctx.begin->range };
    }

    type = peek(ctx)->type;
    if (is_binary_operator(type) && !(special_assignment && type == lexer::token_type::assign))
    {
        auto p1 = precedence({ type, operator_type::binary });
        auto p2 = ctx.operator_stack.size() ? precedence(ctx.operator_stack.back()) : boost::optional<std::size_t>{};
        while (ctx.operator_stack.empty() || p1 < p2 || (p1 == p2 && associativity(type) == assoc::right))
        {
            fmap(ret.expression_value, [&](const auto & value) -> unit { ret.range = value.range; return {}; });
            ret.expression_value = parse_binary_expression(ctx, std::move(ret));

            if (!peek(ctx))
            {
                break;
            }

            type = peek(ctx)->type;

            if (!is_binary_operator(type))
            {
                break;
            }
        }
    }

    fmap(ret.expression_value, [&](const auto & value) -> unit { ret.range = value.range; return {}; });

    return ret;
}

void reaver::vapor::parser::_v1::print(const reaver::vapor::parser::_v1::expression & expr, std::ostream & os, std::size_t indent)
{
    auto in = std::string(indent, ' ');

    os << in << "`expression` at " << expr.range << '\n';
    os << in << "{\n";
    fmap(expr.expression_value, [&](const auto & value) -> unit { print(value, os, indent + 4); return {}; });
    os << in << "}\n";
}
