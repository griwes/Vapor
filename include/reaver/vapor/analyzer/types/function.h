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

#include <boost/algorithm/string/join.hpp>
#include <boost/functional/hash.hpp>

#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function_type : public type
    {
    public:
        function_type(type * ret, std::vector<type *> params);

        virtual std::string explain() const override
        {
            return "function ("
                + boost::join(fmap(_parameters, [&](auto && param) { return param->explain(); }), ", ")
                + ") -> " + _return->explain();
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << explain() << styles::def << " @ " << styles::address
               << this << styles::def << ": builtin function type\n";
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type op) const override;

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
        virtual void _codegen_type(ir_generation_context &) const override
        {
            assert(0);
        }

        type * _return;
        std::vector<type *> _parameters;
        std::unique_ptr<function> _call_operator;
    };

    using function_type_elements = std::pair<type *, std::vector<type *>>;

    struct function_type_elements_hash
    {
        std::size_t operator()(const function_type_elements & elems) const
        {
            auto hash = boost::hash_value(elems.first);
            boost::hash_combine(hash, elems.second);
            return hash;
        }
    };
}
}
