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

#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class overload_set;

    class overload_set_type : public user_defined_type
    {
    public:
        overload_set_type(std::unique_ptr<scope> lex_scope, overload_set * oset)
            : user_defined_type{ std::move(lex_scope) }, _oset{ oset }
        {
        }

        virtual std::string explain() const override
        {
            return "overload set (TODO: add location and member info)";
        }

        virtual void print(std::ostream & os, print_context ctx) const override;
        virtual future<std::vector<function *>> get_candidates(lexer::token_type bracket) const override;

        void mark_exported()
        {
            _is_exported = true;
        }

    private:
        virtual std::unique_ptr<google::protobuf::Message> _user_defined_interface() const override;

        virtual void _codegen_user_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const override;

        overload_set * _oset = nullptr;
        bool _is_exported = false;
    };
}
}
