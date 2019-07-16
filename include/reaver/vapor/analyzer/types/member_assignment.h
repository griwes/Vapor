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

#include "../semantic/function.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class member_assignment_expression;
    class member_assignment_type;

    std::unique_ptr<member_assignment_type> make_member_assignment_type(std::u32string member_name,
        member_assignment_expression * var,
        bool = false);

    class member_assignment_type : public type
    {
    public:
        member_assignment_type(std::u32string member_name,
            member_assignment_expression * expr,
            bool is_assigned = false)
            : _member_name{ std::move(member_name) }, _expr{ expr }, _assigned{ is_assigned }
        {
            if (!_assigned)
            {
                _assigned_type = make_member_assignment_type(_member_name, _expr, true);
            }
        }

        ~member_assignment_type();

        virtual std::string explain() const override
        {
            return "member assignment type for member " + utf8(_member_name);
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << "member assignment type";
            os << styles::def << " @ " << styles::address << this;
            os << styles::def << ": " << styles::string_value << utf8(_member_name) << '\n';
        }

        const std::u32string & member_name() const
        {
            return _member_name;
        }

        type * assigned_type() const
        {
            return _assigned_type.get();
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type) const override;

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const override
        {
            assert(!"attempted to codegen a member-assignment-type");
        }

        virtual std::u32string _codegen_name(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        std::u32string _member_name;
        member_assignment_expression * _expr;

        bool _assigned = false;
        std::unique_ptr<type> _assigned_type;

        mutable std::optional<std::unique_ptr<function>> _fun_storage;
    };

    inline std::unique_ptr<member_assignment_type> make_member_assignment_type(std::u32string member_name,
        member_assignment_expression * var,
        bool is_assigned)
    {
        return std::make_unique<member_assignment_type>(std::move(member_name), var, is_assigned);
    }
}
}
