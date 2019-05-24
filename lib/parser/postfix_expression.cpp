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

#include "vapor/parser/postfix_expression.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/expression_list.h"
#include "vapor/parser/helpers.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    bool operator==(const postfix_expression & lhs, const postfix_expression & rhs)
    {
        return lhs.range == rhs.range && lhs.base_expression == rhs.base_expression
            && lhs.modifier_type == rhs.modifier_type && lhs.arguments == rhs.arguments
            && lhs.accessed_member == rhs.accessed_member;
    }

    postfix_expression parse_postfix_expression(context & ctx, expression_special_modes mode)
    {
        auto get_if_valid_modifier = [&]() -> std::optional<lexer::token_type> {
            for (auto && type : { lexer::token_type::round_bracket_open,
                     lexer::token_type::square_bracket_open,
                     lexer::token_type::curly_bracket_open,
                     lexer::token_type::dot })
            {
                if (type == lexer::token_type::curly_bracket_open && mode == expression_special_modes::brace)
                {
                    continue;
                }

                if (peek(ctx, type))
                {
                    return std::make_optional(type);
                }
            }

            return std::nullopt;
        };

        auto closing = [](lexer::token_type type) {
            using namespace lexer;
            switch (type)
            {
                case token_type::round_bracket_open:
                    return token_type::round_bracket_close;
                case token_type::square_bracket_open:
                    return token_type::square_bracket_close;
                case token_type::curly_bracket_open:
                    return token_type::curly_bracket_close;
                case token_type::dot:
                    return token_type::none;
                default:
                    throw exception(logger::crash) << "invalid modifier type";
            }
        };

        postfix_expression ret;

        position start, end;

        auto type = peek(ctx)->type;

        switch (type)
        {
            case lexer::token_type::round_bracket_open:
                start = expect(ctx, lexer::token_type::round_bracket_open).range.start();
                ret.base_expression = parse_expression_list(ctx);
                end = expect(ctx, lexer::token_type::round_bracket_close).range.end();
                break;

            case lexer::token_type::identifier:
            {
                ret.base_expression = parse_literal<lexer::token_type::identifier>(ctx);
                auto & range = std::get<identifier>(ret.base_expression).range;
                start = range.start();
                end = range.end();
                break;
            }

            case lexer::token_type::default_:
                ret.base_expression = parse_default_instance_expression(ctx);
                break;

            default:
                throw expectation_failure{ "postfix-expression", ctx.begin->string, ctx.begin->range };
        }

        auto modifier = get_if_valid_modifier();

        while (true)
        {
            if (modifier)
            {
                ret.modifier_type = modifier.value();
                expect(ctx, modifier.value());

                if (ret.modifier_type == lexer::token_type::dot)
                {
                    ret.accessed_member = parse_literal<lexer::token_type::identifier>(ctx);
                    end = ret.accessed_member->range.end();
                }

                else
                {
                    if (!peek(ctx, closing(*ret.modifier_type)))
                    {
                        auto old_stack = std::move(ctx.operator_stack);

                        ret.arguments.push_back(parse_expression(ctx));
                        while (peek(ctx, lexer::token_type::comma))
                        {
                            expect(ctx, lexer::token_type::comma);
                            ret.arguments.push_back(parse_expression(ctx));
                        }

                        ctx.operator_stack = std::move(old_stack);
                    }
                    end = expect(ctx, closing(*ret.modifier_type)).range.end();
                }
            }

            ret.range = { start, end };

            modifier = get_if_valid_modifier();

            if (!modifier)
            {
                return ret;
            }

            auto base = std::move(ret);
            ret = {};
            expression_list base_list = { base.range, { expression{ base.range, std::move(base) } } };
            ret.base_expression = std::move(base_list);
        }

        return ret;
    }

    void print(const postfix_expression & expr, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "postfix-expression";
        print_address_range(os, expr);

        auto base_expression_ctx = ctx.make_branch(!expr.modifier_type);
        os << '\n' << base_expression_ctx << styles::subrule_name << "base-expression:\n";
        fmap(expr.base_expression, [&](const auto & value) -> unit {
            print(value, os, base_expression_ctx.make_branch(true));
            return {};
        });

        if (expr.modifier_type)
        {
            auto modifier_ctx =
                ctx.make_branch(expr.modifier_type != lexer::token_type::dot && expr.arguments.empty());
            os << modifier_ctx << styles::subrule_name << "modifier-type: " << styles::def
               << lexer::token_types[+*expr.modifier_type] << '\n';

            if (expr.modifier_type == lexer::token_type::dot)
            {
                os << ctx.make_branch(true) << styles::subrule_name << "accessed-member: '"
                   << styles::string_value << utf8(expr.accessed_member->value.string) << styles::def
                   << "'\n";
            }

            else if (!expr.arguments.empty())
            {
                auto arguments_ctx = ctx.make_branch(true);
                os << arguments_ctx << styles::subrule_name << "arguments:\n";

                std::size_t idx = 0;
                for (auto && arg : expr.arguments)
                {
                    print(arg, os, arguments_ctx.make_branch(++idx == expr.arguments.size()));
                }
            }
        }
    }
}
}
