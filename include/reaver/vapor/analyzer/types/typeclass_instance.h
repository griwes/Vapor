/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019 Michał "Griwes" Dominiak
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

#include "../expressions/member.h"
#include "../expressions/overload_set.h"
#include "../semantic/instance_context.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class typeclass;
    class block;
    class default_instance;

    class typeclass_instance_type : public type, public std::enable_shared_from_this<typeclass_instance_type>
    {
    public:
        friend class typeclass;

        typeclass_instance_type(typeclass * tc, std::vector<type *> arguments);

        virtual std::string explain() const override;
        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual std::unique_ptr<proto::type> generate_interface() const override;
        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override;

        auto & get_overload_sets() const
        {
            return _osets;
        }

        const typeclass * get_typeclass() const
        {
            return _ctx.tc;
        }

        future<std::unique_ptr<expression>> get_default_instance_expr(analysis_context &) const;

    private:
        struct _function_instance
        {
            std::unique_ptr<function> instance;
            std::unique_ptr<block> function_body;
            std::shared_ptr<expression> return_type_expression;
            std::vector<std::unique_ptr<expression>> parameter_expressions;
            std::unique_ptr<class overload_set_expression> overload_set_expression;
        };

        std::unique_ptr<scope> _oset_scope = std::make_unique<scope>();

        std::vector<type *> _arguments;
        instance_context _ctx;

        std::map<std::u32string, overload_set *> _osets;
        std::vector<_function_instance> _function_instances;

        std::vector<std::unique_ptr<member_expression>> _member_expressions;

        future<> _analyze(analysis_context & ctx);

        virtual void _codegen_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const override;
        virtual std::u32string _codegen_name(ir_generation_context &) const override;
    };
}
}

