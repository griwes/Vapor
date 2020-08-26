/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2017, 2019 Michał "Griwes" Dominiak
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
#include <string>
#include <vector>

#include <reaver/variant.h>

#include "function.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    namespace ir
    {
        struct type;

        struct member_variable
        {
            std::u32string name;
            std::shared_ptr<struct type> type;
        };

        using member = std::variant<member_variable, codegen::ir::function>;

        struct user_type;

        struct type
        {
            virtual ~type() = default;

            virtual bool is_fundamental() const
            {
                return true;
            }

            void operator=(const user_type &) = delete;
        };

        struct user_type : type
        {
            user_type(std::u32string name = {}, std::size_t size = {}, std::vector<member> members = {})
                : name{ std::move(name) }, size{ size }, members{ std::move(members) }
            {
            }

            virtual bool is_fundamental() const
            {
                return false;
            }

            user_type & operator=(const user_type &) = default;

            std::u32string name;
            std::size_t size;
            std::vector<member> members;
        };

        struct sized_integer_type : type
        {
            std::size_t integer_size;
        };

        struct function_type : type
        {
            std::shared_ptr<type> return_type;
            std::vector<std::shared_ptr<type>> parameter_types;
        };

        namespace detail
        {
            struct builtin_types_t
            {
                std::shared_ptr<struct type> integer;
                std::shared_ptr<struct type> boolean;
                std::shared_ptr<struct type> type;

                std::shared_ptr<sized_integer_type> sized_integer(std::size_t size) const;
                std::shared_ptr<function_type> function(std::shared_ptr<struct type> return_type,
                    std::vector<std::shared_ptr<struct type>> parameter_types) const;
            };
        }

        inline const auto & builtin_types()
        {
            static auto types = [] {
                detail::builtin_types_t types;
                types.integer = std::make_shared<type>();
                types.boolean = std::make_shared<type>();
                types.type = std::make_shared<type>();
                return types;
            }();

            return types;
        }
    }
}
}
