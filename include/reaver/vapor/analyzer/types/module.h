/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2020 Michał "Griwes" Dominiak
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

#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class entity;

    class module_type : public type
    {
    public:
        module_type(std::unique_ptr<scope> lex_scope, std::string name)
            : type{ std::move(lex_scope) }, _name{ std::move(name) }
        {
        }

        virtual std::string explain() const override
        {
            return "module type for module `" + _name + "`";
        }

        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            assert(0);
        }

        void add_symbol(std::string name, expression * ent, bool is_visible = true);
        void close_scope();

        virtual std::u32string codegen_name() const override
        {
            assert(0);
        }

        virtual bool is_meta() const override
        {
            return true;
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override
        {
            assert(0);
        }

        std::string _name;
    };
}
}
