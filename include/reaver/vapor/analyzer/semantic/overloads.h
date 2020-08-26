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

#pragma once

#include <memory>
#include <vector>

#include <reaver/future.h>

#include "../../lexer/token.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class expression;
    class scope;
    class analysis_context;
    class function;

    future<std::unique_ptr<expression>> select_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        std::vector<expression *> arguments,
        std::vector<function *> possible_overloads,
        expression * base = nullptr);

    future<std::unique_ptr<expression>> resolve_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        expression * lhs,
        expression * rhs,
        lexer::token_type op);

    future<std::unique_ptr<expression>> resolve_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        expression * base_expr,
        lexer::token_type bracket_type,
        std::vector<expression *> arguments);
}
}
