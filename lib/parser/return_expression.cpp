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

#include "vapor/parser/return_expression.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    bool operator==(const return_expression & lhs, const return_expression & rhs)
    {
        return lhs.range == rhs.range && lhs.return_value == rhs.return_value;
    }

    return_expression parse_return_expression(context & ctx)
    {
        return_expression ret;

        auto start = expect(ctx, lexer::token_type::return_).range.start();
        ret.return_value = parse_expression(ctx);

        ret.range = { start, ret.return_value.range.end() };

        return ret;
    }

    void print(const return_expression & ret, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << "return-expression";
        print_address_range(os, ret);
        os << '\n';

        print(ret.return_value, os, ctx.make_branch(true));
    }
}
}
