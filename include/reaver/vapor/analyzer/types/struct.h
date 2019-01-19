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

#include "../expressions/member.h"
#include "../semantic/function.h"
#include "../statements/statement.h"
#include "type.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class declaration;

    class struct_type : public user_defined_type, public std::enable_shared_from_this<struct_type>
    {
    public:
        struct_type(ast_node parse, std::unique_ptr<scope> member_scope, std::vector<std::unique_ptr<declaration>> member_decls);

        ~struct_type();

        void generate_constructors();

        virtual std::string explain() const override
        {
            return "struct type (TODO: trace information!)";
        }

        virtual void print(std::ostream & os, print_context ctx) const override;

        std::vector<declaration *> get_data_member_decls() const
        {
            return fmap(_data_member_declarations, [](auto && ptr) { return ptr.get(); });
        }

        const std::vector<member_expression *> & get_data_members() const
        {
            return _data_members;
        }

        virtual future<function *> get_constructor(std::vector<const expression *>) const override
        {
            return _aggregate_ctor_future.value();
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type op) const override
        {
            if (op != lexer::token_type::curly_bracket_open)
            {
                return make_ready_future(std::vector<function *>{});
            }

            return _aggregate_copy_ctor_future->then([](auto && ctor) { return std::vector<function *>{ ctor }; });
        }

        auto get_ast_info() const
        {
            return std::make_optional(_parse);
        }

        virtual type * get_member_type(const std::u32string & name) const override
        {
            auto it = std::find_if(_data_members.begin(), _data_members.end(), [&](auto && member) { return member->get_name() == name; });
            if (it == _data_members.end())
            {
                return nullptr;
            }

            return (*it)->get_type();
        }

        void mark_imported()
        {
            _is_imported = true;
        }

        void mark_exported()
        {
            _is_exported = true;
        }

    private:
        virtual std::unique_ptr<google::protobuf::Message> _user_defined_interface() const override;
        virtual void _codegen_type(ir_generation_context &) const override;

        ast_node _parse;
        bool _is_imported = false;
        bool _is_exported = false;

        std::vector<std::unique_ptr<declaration>> _data_member_declarations;
        std::vector<member_expression *> _data_members;

        std::unique_ptr<function> _aggregate_ctor;
        mutable std::optional<future<function *>> _aggregate_ctor_future;
        std::optional<manual_promise<function *>> _aggregate_ctor_promise;

        std::unique_ptr<function> _aggregate_copy_ctor;
        mutable std::optional<future<function *>> _aggregate_copy_ctor_future;
        std::optional<manual_promise<function *>> _aggregate_copy_ctor_promise;
        std::unique_ptr<expression> _this_argument;
        std::vector<std::unique_ptr<expression>> _member_copy_arguments;

        virtual std::u32string _codegen_name(ir_generation_context & ctx) const override
        {
            return get_name();
        }
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct struct_literal;
}
}

namespace reaver::vapor::proto
{
struct struct_type;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<struct_type> make_struct_type(precontext & ctx, const parser::struct_literal & parse, scope * lex_scope);
    std::unique_ptr<struct_type> import_struct_type(precontext & ctx, const proto::struct_type &);
}
}
