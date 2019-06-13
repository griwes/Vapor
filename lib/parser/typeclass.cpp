/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017, 2019 Michał "Griwes" Dominiak
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

#include "vapor/parser/typeclass.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    bool operator==(const typeclass_literal & lhs, const typeclass_literal & rhs)
    {
        return lhs.range == rhs.range && lhs.members == rhs.members;
    }

    bool operator==(const instance_literal & lhs, const instance_literal & rhs)
    {
        return lhs.range == rhs.range && lhs.typeclass_name == rhs.typeclass_name
            && lhs.arguments == rhs.arguments && lhs.definitions == rhs.definitions;
    }

    bool operator==(const default_instance_definition & lhs, const default_instance_definition & rhs)
    {
        return lhs.range == rhs.range && lhs.literal == rhs.literal;
    }

    namespace
    {
        struct named_typeclass
        {
            std::optional<lexer::token> export_;
            std::optional<identifier> name;
            typeclass_literal definition;
        };

        enum class typeclass_type
        {
            named,
            unnamed
        };

        named_typeclass parse_typeclass(context & ctx, typeclass_type type)
        {
            named_typeclass ret;

            if (type == typeclass_type::named && peek(ctx, lexer::token_type::export_))
            {
                ret.export_ = expect(ctx, lexer::token_type::export_);
            }

            auto start = expect(ctx, lexer::token_type::typeclass).range.start();

            if (type == typeclass_type::named)
            {
                ret.name = parse_literal<lexer::token_type::identifier>(ctx);
            }

            expect(ctx, lexer::token_type::round_bracket_open);
            ret.definition.parameters = parse_parameter_list(ctx, parameter_type_mode::required);
            expect(ctx, lexer::token_type::round_bracket_close);

            expect(ctx, lexer::token_type::curly_bracket_open);

            while (!peek(ctx, lexer::token_type::curly_bracket_close))
            {
                ret.definition.members.push_back(parse_function(ctx));
            }

            auto end = expect(ctx, lexer::token_type::curly_bracket_close).range.end();

            ret.definition.range = { start, end };

            return ret;
        }
    }

    typeclass_literal parse_typeclass_literal(context & ctx)
    {
        return parse_typeclass(ctx, typeclass_type::unnamed).definition;
    }

    declaration parse_typeclass_definition(context & ctx)
    {
        auto typeclass = parse_typeclass(ctx, typeclass_type::named);

        declaration decl;

        decl.identifier = std::move(typeclass.name.value());
        decl.range = typeclass.definition.range;
        decl.rhs = expression{ typeclass.definition.range, std::move(typeclass.definition) };
        decl.export_ = typeclass.export_;

        return decl;
    }

    instance_literal parse_instance_literal(context & ctx)
    {
        instance_literal ret;

        auto start = expect(ctx, lexer::token_type::instance).range.start();
        ret.typeclass_name = parse_id_expression(ctx);

        expect(ctx, lexer::token_type::round_bracket_open);
        while (true)
        {
            ret.arguments.push_back(parse_expression(ctx));
            if (!peek(ctx, lexer::token_type::comma))
            {
                break;
            }
        }
        expect(ctx, lexer::token_type::round_bracket_close);

        expect(ctx, lexer::token_type::curly_bracket_open);
        while (!peek(ctx, lexer::token_type::curly_bracket_close))
        {
            ret.definitions.push_back(parse_function_definition(ctx, parameter_type_mode::optional));
        }
        auto end = expect(ctx, lexer::token_type::curly_bracket_close).range.end();

        ret.range = { start, end };

        return ret;
    }

    default_instance_definition parse_default_instance(context & ctx)
    {
        default_instance_definition ret;

        auto start = expect(ctx, lexer::token_type::default_).range.start();
        ret.literal = parse_instance_literal(ctx);
        ret.range = { start, ret.literal.range.end() };

        return ret;
    }

    void print(const typeclass_literal & lit, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "typeclass-literal";
        print_address_range(os, lit);
        os << '\n';

        auto param_ctx = ctx.make_branch(false);
        os << styles::def << param_ctx << styles::subrule_name << "parameters:\n";
        print(lit.parameters, os, param_ctx.make_branch(true));

        auto defs_ctx = ctx.make_branch(true);
        os << styles::def << defs_ctx << styles::subrule_name << "members:\n";

        std::size_t idx = 0;
        for (auto && member : lit.members)
        {
            fmap(member, [&](auto && decl) {
                print(decl, os, defs_ctx.make_branch(++idx == lit.members.size()));
                return unit{};
            });
        }
    }

    void print(const instance_literal & lit, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "instance-literal";
        print_address_range(os, lit);
        os << '\n';

        auto name_ctx = ctx.make_branch(false);
        os << styles::def << name_ctx << styles::subrule_name << "typeclass-name:\n";
        print(lit.typeclass_name, os, name_ctx.make_branch(true));

        auto args_ctx = ctx.make_branch(false);
        os << styles::def << args_ctx << styles::subrule_name << "arguments:\n";
        std::size_t idx = 0;
        for (auto && arg : lit.arguments)
        {
            print(arg, os, args_ctx.make_branch(++idx == lit.arguments.size()));
        }

        auto defs_ctx = ctx.make_branch(true);
        os << styles::def << defs_ctx << styles::subrule_name << "definitions:\n";

        idx = 0;
        for (auto && def : lit.definitions)
        {
            fmap(def, [&](auto && def) {
                print(def, os, defs_ctx.make_branch(++idx == lit.definitions.size()));
                return unit{};
            });
        }
    }

    void print(const default_instance_definition & def, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "default-instance-definition";
        print_address_range(os, def);
        os << '\n';

        auto literal_ctx = ctx.make_branch(true);
        os << styles::def << literal_ctx << styles::subrule_name << "instance-literal:\n";
        print(def.literal, os, literal_ctx.make_branch(true));
    }
}
}
