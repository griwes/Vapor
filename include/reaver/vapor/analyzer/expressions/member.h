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

#include <string>

#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class member_expression : public expression
    {
    public:
        member_expression(type * parent_type, std::u32string name, type * own_type)
            : expression{ own_type, parent_type ? parent_type->get_scope() : nullptr, std::move(name) },
              _parent{ parent_type }
        {
        }

        codegen::ir::member_variable member_codegen_ir(ir_generation_context & ctx) const;

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "member-expression";
            os << styles::def << " @ " << styles::address << this << styles::def << ": ";
            os << styles::string_value << utf8(get_name().value()) << styles::def << '\n';

            auto type_ctx = ctx.make_branch(false);
            os << styles::def << type_ctx << styles::subrule_name << "type:\n";
            get_type()->print(os, type_ctx.make_branch(true));

            auto parent_ctx = ctx.make_branch(true);
            if (_parent)
            {
                os << styles::def << parent_ctx << styles::subrule_name << "parent type:\n";
                _parent->print(os, parent_ctx.make_branch(true));
            }
            else
            {
                os << styles::def << parent_ctx << styles::subrule_name << "unknown parent type\n";
            }
        }

        virtual bool is_member() const override
        {
            return true;
        }

        virtual void set_parent_type(type * parent_type)
        {
            assert(!_parent);
            assert(parent_type);
            _parent = parent_type;
            _set_scope(parent_type->get_scope());
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override
        {
            return std::make_unique<member_expression>(_parent, get_name().value(), get_type());
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        virtual bool _is_equal(const expression * rhs) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        type * _parent = nullptr;
    };

    inline auto make_member_expression(type * parent, std::u32string name, type * own_type)
    {
        return std::make_unique<member_expression>(parent, std::move(name), own_type);
    }
}
}
