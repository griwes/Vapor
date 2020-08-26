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

#include "../types/member_assignment.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class member_assignment_expression : public expression
    {
    public:
        member_assignment_expression(std::u32string member_name)
            : expression{ nullptr, std::nullopt },
              _type{ make_member_assignment_type(std::move(member_name), this) }
        {
            _set_type(_type.get());
        }

        const std::u32string member_name() const
        {
            return _type->member_name();
        }

        type * get_assigned_type() const
        {
            assert(_rhs);
            return _rhs->get_type();
        }

        void set_rhs(expression * rhs)
        {
            assert(!_rhs);
            _rhs = rhs;
        }

        auto get_rhs() const
        {
            assert(_rhs);
            return _rhs;
        }

        virtual bool is_member_assignment() const override
        {
            return true;
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "member-assignment-expression";
            os << styles::def << " @ " << styles::address << this << styles::def << ":\n";

            auto type_ctx = ctx.make_branch(false);
            os << styles::def << type_ctx << styles::subrule_name << "assignment-type:\n";
            _type->print(os, type_ctx.make_branch(true));

            auto rhs_ctx = ctx.make_branch(true);
            os << styles::def << rhs_ctx << styles::subrule_name << "right-hand side:\n";
            _rhs->print(os, rhs_ctx.make_branch(true));
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            auto ret = std::make_unique<member_assignment_expression>(_type->member_name());
            ret->_rhs = _rhs;

            if (auto replaced = repl.try_get_replacement(_rhs))
            {
                ret->_rhs = replaced;
            }

            return ret;
        }

        virtual future<expression *> _simplify_expr(recursive_context ctx) override
        {
            return _rhs->simplify_expr(ctx).then([&](auto && simpl) -> expression * {
                _rhs = simpl;
                return this;
            });
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            assert(0);
            // return _rhs->codegen_ir(ctx);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        expression * _rhs = nullptr;
        std::unique_ptr<expression> _owned_expr;

        std::unique_ptr<member_assignment_type> _type;
    };

    inline auto make_member_assignment_expression(std::u32string member_name)
    {
        return std::make_unique<member_assignment_expression>(std::move(member_name));
    }
}
}
