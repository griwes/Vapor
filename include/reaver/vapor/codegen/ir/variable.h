/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2019 Michał "Griwes" Dominiak
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
#include <variant>
#include <vector>

#include <reaver/variant.h>

#include "../../utf.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    namespace ir
    {
        struct type;
        struct value;

        struct variable
        {
            variable(std::shared_ptr<struct type> type, std::optional<std::u32string> name)
                : type{ std::move(type) }, name{ std::move(name) }
            {
            }

            std::shared_ptr<struct type> type;
            std::optional<std::u32string> name;
            bool declared = false;
            bool destroyed = false;
            bool parameter = false;
            bool imported = false;
            bool constant = false;

            std::optional<recursive_wrapper<value>> initializer;
        };

        struct label
        {
            std::u32string name;
        };

        inline std::shared_ptr<variable> make_variable(std::shared_ptr<struct type> type,
            std::optional<std::u32string> name = std::nullopt)
        {
            return std::make_shared<variable>(variable{ std::move(type), std::move(name) });
        }
    }
}
}
