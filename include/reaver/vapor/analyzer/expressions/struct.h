/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2020 Michał "Griwes" Dominiak
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

#include "../types/struct.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class struct_expression : public expression
    {
    public:
        struct_expression(std::shared_ptr<struct_type> type, std::vector<std::unique_ptr<expression>> fields)
            : expression{ type.get(), type->get_scope()->parent(), std::nullopt }, _type{ type }
        {
            auto members = _type->get_data_members();

            assert(fields.size() == members.size());

            _fields_in_order.reserve(fields.size());

            for (std::size_t i = 0; i < fields.size(); ++i)
            {
                _fields_in_order.push_back(fields[i].get());
                _fields[members[i]] = std::move(fields[i]);
            }
        }

        virtual bool is_constant() const override
        {
            return std::all_of(_fields_in_order.begin(), _fields_in_order.end(), [](auto && field) {
                return field->is_constant();
            });
        }

        virtual expression * get_member(const std::u32string & name) const override
        {
            auto it = std::find_if(
                _fields.begin(), _fields.end(), [&](auto && elem) { return elem.first->get_name() == name; });
            if (it == _fields.end())
            {
                return nullptr;
            }

            return it->second.get();
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "struct-expression";
            os << styles::def << " @ " << styles::address << this << styles::def << ":\n";

            auto type_ctx = ctx.make_branch(_fields_in_order.empty());
            os << styles::def << type_ctx << styles::subrule_name << "type:\n";
            _type->print(os, type_ctx.make_branch(true));

            if (!_fields_in_order.empty())
            {
                auto members_ctx = ctx.make_branch(true);
                os << styles::def << members_ctx << styles::subrule_name << "member values:\n";

                std::size_t idx = 0;
                for (auto && member : _fields_in_order)
                {
                    member->print(os, members_ctx.make_branch(++idx == _fields_in_order.size()));
                }
            }
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return std::make_unique<struct_expression>(
                _type, fmap(_fields_in_order, [&](auto && field) { return repl.copy_claim(field); }));
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            auto ir = fmap(_fields_in_order, [&](auto && field) { return field->codegen_ir(ctx); });
            auto type = std::dynamic_pointer_cast<codegen::ir::user_type>(_type->codegen_type(ctx));
            auto result = codegen::ir::struct_value{ std::move(type),
                fmap(ir, [&](auto && field_ir) { return field_ir.back().result; }) };

            return { codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() },
                {},
                std::move(result) } };
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual bool _is_equal(const expression * rhs) const override
        {
            auto rhs_struct = rhs->as<struct_expression>();
            return rhs_struct && _type == rhs_struct->_type && _fields == rhs_struct->_fields;
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        std::shared_ptr<struct_type> _type;
        std::unordered_map<const member_expression *, std::unique_ptr<expression>> _fields;
        std::vector<expression *> _fields_in_order;
    };

    inline auto make_struct_expression(std::shared_ptr<struct_type> type,
        std::vector<std::unique_ptr<expression>> fields)
    {
        return std::make_unique<struct_expression>(type, std::move(fields));
    }
}
}
