/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2017, 2019 Michał "Griwes" Dominiak
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

#pragma once

#include <optional>

#include "../print_helpers.h"
#include "../range.h"
#include "helpers.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    template<lexer::token_type TokenType>
    struct literal
    {
        range_type range;
        lexer::token value;
        std::optional<lexer::token> suffix = {};
    };

    template<lexer::token_type TokenType>
    bool operator==(const literal<TokenType> & lhs, const literal<TokenType> & rhs)
    {
        return lhs.range == rhs.range && lhs.value == rhs.value && lhs.suffix == rhs.suffix;
    }

    using string_literal = literal<lexer::token_type::string>;
    using integer_literal = literal<lexer::token_type::integer>;
    using boolean_literal = literal<lexer::token_type::boolean>;

    using identifier = literal<lexer::token_type::identifier>;

    template<lexer::token_type TokenType, lexer::token_type ToExpect = TokenType>
    auto parse_literal(context & ctx)
    {
        literal<TokenType> ret;

        if constexpr (TokenType == ToExpect)
        {
            ret.value = expect(ctx, TokenType);
        }
        else
        {
            ret.value = expect(ctx, ToExpect);
            ret.value.type = ToExpect;
        }

        auto start = ret.value.range.start();
        auto end = ret.value.range.end();

        if (peek(ctx, lexer::suffix(TokenType)))
        {
            ret.suffix = expect(ctx, lexer::suffix(TokenType));
            end = ret.suffix->range.end();
        }

        ret.range = { start, end };

        return ret;
    }

    template<lexer::token_type TokenType>
    void print(const literal<TokenType> & lit, std::ostream & os, print_context ctx)
    {
        os << styles::def << ctx << styles::rule_name << lexer::token_types[+lit.value.type];
        print_address_range(os, lit);
        os << " '" << styles::string_value << utf8(lit.value.string) << styles::def << "'";

        assert(!lit.suffix);

        os << '\n';
    }

    inline void print(const lexer::token & tok, std::ostream & os, print_context ctx)
    {
        os << ctx << tok << '\n';
    }
}
}
