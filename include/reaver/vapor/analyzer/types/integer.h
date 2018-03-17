/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2018 Michał "Griwes" Dominiak
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

#include "../../lexer/token.h"
#include "../function.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class integer_type : public type
    {
    public:
        integer_type() : type{ dont_init_expr }
        {
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type token) const override
        {
            auto ret = [](auto && fn) { return make_ready_future(std::vector<function *>{ fn }); };

            switch (token)
            {
                case lexer::token_type::plus:
                    return ret(_addition());

                case lexer::token_type::minus:
                    return ret(_subtraction());

                case lexer::token_type::star:
                    return ret(_multiplication());

                case lexer::token_type::equals:
                    return ret(_equal_comparison());

                case lexer::token_type::less:
                    return ret(_less_comparison());

                case lexer::token_type::less_equal:
                    return ret(_less_equal_comparison());

                default:
                    assert(!"unimplemented int op");
            }
        }

        virtual std::string explain() const override
        {
            return "integer";
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << "integer" << styles::def << " @ " << styles::address << this << styles::def << ": builtin type\n";
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override;

        virtual std::u32string _codegen_name(ir_generation_context & ctx) const override
        {
            return U"integer";
        }

        template<typename Instruction, typename Eval>
        static auto _generate_function(const char32_t * name, const char * desc, Eval eval, type * return_type);
        static function * _addition();
        static function * _subtraction();
        static function * _multiplication();
        static function * _equal_comparison();
        static function * _less_comparison();
        static function * _less_equal_comparison();
    };

    std::unique_ptr<type> make_integer_type();
}
}
