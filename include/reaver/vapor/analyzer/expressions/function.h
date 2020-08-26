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

#include "../expressions/expression.h"
#include "../semantic/function.h"
#include "../types/function.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function_expression : public expression
    {
    public:
        function_expression(function * fun,
            function_type * type,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : expression{ type, lex_scope, std::move(name) }, _fun{ fun }, _type{ type }
        {
        }

        function * get_value() const
        {
            return _fun;
        }

        bool is_constant() const override
        {
            return true;
        }

        virtual void print(std::ostream &, print_context) const override
        {
            assert(0);
        }

    private:
        virtual future<> _analyze(analysis_context & ctx) override
        {
            if (_type)
            {
                return make_ready_future();
            }

            return _fun->get_return_type().then([=, &ctx](auto && return_type_expr) {
                assert(return_type_expr->get_type() == builtin_types().type);

                auto return_type = [&] {
                    if (return_type_expr->is_constant())
                    {
                        auto return_type_type_expr = return_type_expr->template as<type_expression>();
                        assert(return_type_type_expr);
                        return return_type_type_expr->get_value();
                    }

                    return builtin_types().unconstrained;
                }();

                auto param_types = fmap(_fun->parameters(), [](auto && expr) { return expr->get_type(); });

                _set_type(ctx.get_function_type({ std::move(param_types), return_type }));
            });
        }

        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return std::unique_ptr<expression>(new function_expression(_fun, _type, get_scope(), get_name()));
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        function * _fun;
        function_type * _type;
    };

    inline auto make_function_expression(function * fun,
        function_type * type,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        return std::make_unique<function_expression>(fun, type, lex_scope, std::move(name));
    }
}
}
