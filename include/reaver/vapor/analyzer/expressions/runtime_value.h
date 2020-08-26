/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class runtime_value_expression : public expression
    {
    public:
        using expression::expression;

        virtual void print(std::ostream &, print_context) const override
        {
            assert(0);
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override
        {
            return std::make_unique<runtime_value_expression>(get_type(), get_scope(), get_name());
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            return { codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() },
                {},
                codegen::ir::make_variable(get_type()->codegen_type(ctx)) } };
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }
    };

    inline std::unique_ptr<expression> make_runtime_value(type * t,
        scope * lex_scope,
        std::optional<std::u32string> name = std::nullopt)
    {
        return std::make_unique<runtime_value_expression>(t, lex_scope, std::move(name));
    }
}
}
