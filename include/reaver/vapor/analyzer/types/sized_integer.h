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

#include "../semantic/function.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class sized_integer : public type
    {
    public:
        sized_integer(std::size_t size);

        virtual std::string explain() const override
        {
            return "sized_integer(" + std::to_string(_size) + ")";
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << "sized_integer(" << std::to_string(_size) << ")"
               << styles::def << " @ " << styles::address << this << styles::def << ": builtin type\n";
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type token) const override
        {
            auto ret = [](auto && fn) { return make_ready_future(std::vector<function *>{ fn.get() }); };

            switch (token)
            {
                case lexer::token_type::plus:
                    return ret(_addition);

                case lexer::token_type::minus:
                    return ret(_subtraction);

                case lexer::token_type::star:
                    return ret(_multiplication);

                case lexer::token_type::slash:
                    return ret(_division);

                case lexer::token_type::equals:
                    return ret(_equal_comparison);

                case lexer::token_type::less:
                    return ret(_less_comparison);

                case lexer::token_type::less_equal:
                    return ret(_less_equal_comparison);

                default:
                    assert(!"unimplemented int op");
            }
        }

        virtual bool matches(type * other) const override
        {
            if (auto other_sized = dynamic_cast<sized_integer *>(other))
            {
                return _size <= other_sized->_size;
            }

            return other == builtin_types().integer;
        }

        virtual bool needs_conversion(type * other) const override
        {
            return this != other;
        }

        auto max_value() const
        {
            return _max_value;
        }

        auto min_value() const
        {
            return _min_value;
        }

        auto size() const
        {
            return _size;
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override;
        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override;

        virtual std::u32string codegen_name() const override
        {
            return U"sized_int(" + utf32(std::to_string(_size)) + U")";
        }

    private:
        virtual void _codegen_type(ir_generation_context & ctx) const override;

        std::size_t _size;
        boost::multiprecision::cpp_int _max_value;
        boost::multiprecision::cpp_int _min_value;

        template<typename Instruction, typename Eval>
        auto _generate_function(std::u32string name, std::string desc, Eval eval, type * return_type);
        std::unique_ptr<function> _addition;
        std::unique_ptr<function> _subtraction;
        std::unique_ptr<function> _multiplication;
        std::unique_ptr<function> _division;
        std::unique_ptr<function> _equal_comparison;
        std::unique_ptr<function> _less_comparison;
        std::unique_ptr<function> _less_equal_comparison;
    };

    inline std::unique_ptr<sized_integer> make_sized_integer_type(std::size_t size)
    {
        return std::make_unique<sized_integer>(size);
    }
}
}
