/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2020 Michał "Griwes" Dominiak
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
    class pack_type : public type
    {
    public:
        pack_type(type * pattern) : type{ dont_init_pack }, _pattern{ pattern }
        {
        }

        virtual std::string explain() const override
        {
            return _pattern->explain() + "...";
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "pack type of:\n";
            _pattern->print(os, ctx.make_branch(true));
        }

        virtual bool matches(type * other) const override
        {
            return _pattern->matches(other);
        }

        virtual bool matches(const std::vector<type *> & others) const override
        {
            return std::all_of(
                others.begin(), others.end(), [&](auto && other) { return _pattern->matches(other); });
        }

        virtual type * get_pack_type() const override
        {
            assert(!"tried to make a pack of packs, this is nonsense");
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            assert(0);
        }

        virtual std::u32string codegen_name() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_type(ir_generation_context & ctx) const override
        {
            assert(!"tried to codegen a type pack");
        }

        type * _pattern;
    };

    inline std::unique_ptr<type> make_pack_type(type * pattern)
    {
        return std::make_unique<pack_type>(pattern);
    }
}
}
