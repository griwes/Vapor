/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019-2020 Michał "Griwes" Dominiak
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

#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class typeclass;

    class archetype : public type
    {
    public:
        archetype(ast_node node, const type * base, std::u32string param_name);

        auto get_ast_info() const
        {
            return std::make_optional(_node);
        }

        virtual std::string explain() const override;
        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override;

        virtual std::u32string codegen_name() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override
        {
            assert(0);
        }

        ast_node _node;
        std::u32string _param_name;

        const type * _base_type;
        std::vector<typeclass *> _typeclasses;
    };
}
}
