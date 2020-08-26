/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2019 Michał "Griwes" Dominiak
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

#include "../types/unresolved.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class unresolved_type_expression : public expression
    {
    public:
        unresolved_type_expression(std::shared_ptr<unresolved_type> type,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : expression{ lex_scope, std::move(name) }, _type{ std::move(type) }
        {
        }

        virtual void print(std::ostream &, print_context) const override
        {
            assert(0);
        }

    private:
        virtual future<> _analyze(analysis_context & ctx) override
        {
            return _type->resolve(ctx);
        }

        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            auto type = _type->get_resolved();
            assert(type);
            return repl.copy_claim(type->get_expression());
        }

        virtual statement_ir _codegen_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        template<typename T>
        static auto _get_replacement_impl(T & t)
        {
            auto type = t._type->get_resolved();
            return type ? type->get_expression() : &t;
        }

        virtual expression * _get_replacement() override
        {
            return _get_replacement_impl(*this);
        }

        virtual const expression * _get_replacement() const override
        {
            return _get_replacement_impl(*this);
        }

        std::shared_ptr<unresolved_type> _type;
    };
}
}
