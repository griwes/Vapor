/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2018 Michał "Griwes" Dominiak
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
    class unconstrained_type : public type
    {
    public:
        unconstrained_type() : type{ dont_init_expr }
        {
        }

        virtual std::string explain() const override
        {
            return "unconstrained type pattern";
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << "<unconstrained>" << styles::def << " @ " << styles::address << this << styles::def
               << ": builtin type\n";
        }

        virtual bool matches(type * other) const override
        {
            return true;
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override
        {
            assert(!"tried to codegen a type pattern!");
        }

        virtual std::u32string _codegen_name(ir_generation_context & ctx) const override
        {
            assert(0);
        }
    };

    std::unique_ptr<type> make_unconstrained_type();
}
}
