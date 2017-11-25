/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2017 Michał "Griwes" Dominiak
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

#include <string>

#include "../range.h"
#include "expression_list.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    using ident_type = identifier;

    struct declaration
    {
        range_type range;
        ident_type identifier;
        std::optional<expression> type_expression;
        std::optional<expression> rhs;
    };

    enum class declaration_mode
    {
        variable_declaration,
        member_declaration
    };

    bool operator==(const declaration & lhs, const declaration & rhs);

    declaration parse_declaration(context & ctx, declaration_mode mode = declaration_mode::variable_declaration);

    void print(const declaration & decl, std::ostream & os, print_context ctx);
}
}
