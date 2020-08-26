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

#include <memory>
#include <numeric>

#include "../expressions/closure.h"
#include "../semantic/function.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class closure_type : public user_defined_type
    {
    public:
        closure_type(class closure * closure, std::unique_ptr<scope> lex_scope, std::unique_ptr<function> fn)
            : user_defined_type{ std::move(lex_scope) },
              _closure{ std::move(closure) },
              _function{ std::move(fn) }
        {
        }

        virtual std::string explain() const override
        {
            return "closure (TODO: location)";
        }

        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual future<std::vector<function *>> get_candidates(lexer::token_type bracket) const override;

        auto get_ast_info() const
        {
            return _closure->get_ast_info();
        }

        virtual std::unique_ptr<google::protobuf::Message> _user_defined_interface() const override
        {
            assert(0);
        }

    private:
        virtual void _codegen_user_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const override;

        closure * _closure;
        std::unique_ptr<function> _function;
    };
}
}
