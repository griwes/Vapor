/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019 Michał "Griwes" Dominiak
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

#include "boolean.h"
#include "integer.h"
#include "struct.h"
#include "variable.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    namespace ir
    {
        struct function_type;

        struct function_value
        {
            std::u32string name;
            std::shared_ptr<function_type> type;
        };

        struct value : public std::variant<std::shared_ptr<variable>,
                           std::shared_ptr<type>,
                           integer_value,
                           boolean_value,
                           struct_value,
                           function_value,
                           label>
        {
            using variant::variant;
        };

        std::shared_ptr<type> get_type(const value &);
    }
}
}
