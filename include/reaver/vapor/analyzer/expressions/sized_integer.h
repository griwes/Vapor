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

#include <memory>

#include <boost/multiprecision/integer.hpp>

#include "../types/sized_integer.h"
#include "constant.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class sized_integer_constant : public constant
    {
    public:
        sized_integer_constant(sized_integer * type,
            boost::multiprecision::cpp_int value,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : constant{ type, lex_scope, std::move(name) }, _value{ std::move(value) }, _type{ type }
        {
            assert(_value <= _type->max_value());
            assert(_value >= _type->min_value());
        }

        auto get_value() const
        {
            return _value;
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "sized-integer-constant(" << _type->size()
               << ")";
            print_address_range(os, this);
            os << ' ' << styles::string_value << _value << '\n';
        }

        virtual std::size_t hash_value() const override
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, _type->size());
            boost::hash_combine(seed, _value);
            return seed;
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override
        {
            return std::make_unique<sized_integer_constant>(_type, _value, get_scope(), get_name());
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        virtual bool _is_equal(const expression * rhs) const override
        {
            auto other = rhs->as<sized_integer_constant>();
            if (!other)
            {
                // todo: conversions somehow
                return false;
            }

            return other->get_value() == _value;
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        boost::multiprecision::cpp_int _value;
        sized_integer * _type;
    };
}
}
