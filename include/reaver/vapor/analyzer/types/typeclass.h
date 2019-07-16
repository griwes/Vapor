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

#include "../semantic/instance_context.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class typeclass_type : public type
    {
    public:
        typeclass_type(std::vector<type *> param_types);

        virtual bool is_meta() const override
        {
            return true;
        }

        virtual std::string explain() const override;
        virtual void print(std::ostream & os, print_context ctx) const override;
        virtual future<std::vector<function *>> get_candidates(lexer::token_type) const override;
        virtual std::unique_ptr<proto::type> generate_interface() const override;
        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override;

    private:
        virtual void _codegen_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const override
        {
            assert(0);
        }

        virtual std::u32string _codegen_name(ir_generation_context & ctx) const override
        {
            assert(0);
        }

        std::vector<type *> _param_types;
        std::unique_ptr<function> _call_operator;
    };
}
}

