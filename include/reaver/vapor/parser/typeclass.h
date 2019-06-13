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

#include "declaration.h"
#include "function.h"
#include "helpers.h"
#include "id_expression.h"
#include "parameter_list.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct typeclass_literal
    {
        range_type range;
        parameter_list parameters;
        std::vector<std::variant<function_declaration, function_definition>> members;
    };

    struct instance_literal
    {
        range_type range;
        id_expression typeclass_name;
        std::vector<expression> arguments;
        std::vector<std::variant<function_definition>> definitions;
    };

    struct default_instance_definition
    {
        range_type range;
        instance_literal literal;
    };

    bool operator==(const typeclass_literal & lhs, const typeclass_literal & rhs);
    bool operator==(const instance_literal & lhs, const instance_literal & rhs);
    bool operator==(const default_instance_definition & lhs, const default_instance_definition & rhs);

    typeclass_literal parse_typeclass_literal(context & ctx);
    declaration parse_typeclass_definition(context & ctx);
    instance_literal parse_instance_literal(context & ctx);
    default_instance_definition parse_default_instance(context & ctx);

    void print(const typeclass_literal & lit, std::ostream & os, print_context ctx);
    void print(const instance_literal & def, std::ostream & os, print_context ctx);
    void print(const default_instance_definition & def, std::ostream & os, print_context ctx);
}
}
