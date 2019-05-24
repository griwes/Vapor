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

#include "vapor/parser/expression.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    bool operator==(const expression & lhs, const expression & rhs)
    {
        return lhs.range == rhs.range && lhs.expression_value == rhs.expression_value;
    }

    expression parse_expression(context & ctx, expression_special_modes mode)
    {
        expression ret;

        if (!peek(ctx))
        {
            throw expectation_failure{ "expression" };
        }

        auto type = peek(ctx)->type;

        switch (type)
        {
            case lexer::token_type::string:
                ret.expression_value = parse_literal<lexer::token_type::string>(ctx);
                break;

            case lexer::token_type::integer:
                ret.expression_value = parse_literal<lexer::token_type::integer>(ctx);
                break;

            case lexer::token_type::boolean:
                ret.expression_value = parse_literal<lexer::token_type::boolean>(ctx);
                break;

            case lexer::token_type::identifier:
            case lexer::token_type::default_:
                ret.expression_value = parse_postfix_expression(ctx, mode);
                break;

            case lexer::token_type::import:
                ret.expression_value = parse_import_expression(ctx);
                break;

            case lexer::token_type::lambda:
                ret.expression_value = parse_lambda_expression(ctx);
                break;

            case lexer::token_type::struct_:
                ret.expression_value = parse_struct_literal(ctx);
                break;

            case lexer::token_type::dot:
                ret.expression_value = parse_member_expression(ctx);
                break;

            case lexer::token_type::typeclass:
                ret.expression_value = parse_typeclass_literal(ctx);
                break;

            case lexer::token_type::instance:
                ret.expression_value = parse_instance_literal(ctx);
                break;

            default:
                if (is_unary_operator(type))
                {
                    ret.expression_value = parse_unary_expression(ctx);
                }

                else
                {
                    throw expectation_failure{ "expression", ctx.begin->string, ctx.begin->range };
                }
        }

        if (peek(ctx))
        {
            type = peek(ctx)->type;
            if (is_binary_operator(type)
                && !(mode == expression_special_modes::assignment && type == lexer::token_type::assign))
            {
                auto p1 = precedence({ type, operator_type::binary });
                auto p2 = ctx.operator_stack.size()
                    ? std::make_optional(precedence(ctx.operator_stack.back()))
                    : std::nullopt;
                while (ctx.operator_stack.empty() || p1 < *p2
                    || (p1 == *p2 && associativity(type) == assoc::right))
                {
                    fmap(ret.expression_value, [&](const auto & value) -> unit {
                        ret.range = value.range;
                        return {};
                    });
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

                    p1 = precedence({ type, operator_type::binary });
                }
            }
        }

        fmap(ret.expression_value, [&](const auto & value) -> unit {
            ret.range = value.range;
            return {};
        });

        return ret;
    }

    void print(const expression & expr, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "expression";
        print_address_range(os, expr);
        os << "\n";

        fmap(expr.expression_value, [&](const auto & value) -> unit {
            print(value, os, ctx.make_branch(true));
            return {};
        });
    }
}
}
