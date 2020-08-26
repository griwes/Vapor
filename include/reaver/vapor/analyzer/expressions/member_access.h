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

#include "expression.h"
#include "expression_ref.h"
#include "member_assignment.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class member_access_expression : public expression
    {
    public:
        member_access_expression(ast_node parse,
            scope * lex_scope,
            std::optional<std::u32string> name,
            std::u32string referenced_name)
            : expression{ lex_scope, std::move(name) }, _referenced_name{ std::move(referenced_name) }
        {
            _set_ast_info(parse);
        }

        member_access_expression(scope * lex_scope,
            std::optional<std::u32string> name,
            std::u32string referenced_name,
            type * referenced_type)
            : expression{ referenced_type, lex_scope, std::move(name) },
              _referenced_name{ std::move(referenced_name) }
        {
            assert(referenced_type);
        }

        virtual void print(std::ostream & os, print_context) const override;

        auto get_referenced_name() const
        {
            return _referenced_name;
        }

        virtual bool is_member_access() const override
        {
            return true;
        }

        virtual bool is_constant() const override
        {
            return _referenced && _referenced->is_constant();
        }

        void set_base_expression(expression * base);

    private:
        member_access_expression(ast_node parse,
            scope * lex_scope,
            std::optional<std::u32string> name,
            expression * referenced)
            : expression{ lex_scope, name }, _referenced{ referenced }
        {
            _set_ast_info(parse);
        }

        virtual future<> _analyze(analysis_context &) override;

        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            if (_referenced)
            {
                return make_expression_ref(
                    repl.get_replacement(_referenced), get_scope(), get_name(), get_ast_info());
            }

            if (!_base)
            {
                return std::unique_ptr<member_access_expression>{ new member_access_expression{
                    get_scope(), get_name(), _referenced_name, get_type() } };
            }

            auto replaced_base = repl.get_replacement(_base);
            if (auto repl = replaced_base->get_member(_referenced_name))
            {
                return make_expression_ref(repl, get_scope(), get_name(), get_ast_info());
            }

            assert(!_assignment_expr);

            std::unique_ptr<member_access_expression> ret{ new member_access_expression{
                get_ast_info().value(), get_scope(), get_name(), nullptr } };
            ret->_base = replaced_base;
            ret->_set_type(get_type());
            ret->_referenced_name = _referenced_name;
            return ret;
        }

        virtual future<expression *> _simplify_expr(recursive_context) override
        {
            if (_referenced && _referenced->is_constant())
            {
                return make_ready_future(_referenced);
            }
            return make_ready_future<expression *>(this);
        }

        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;
        virtual bool _invalidate_ir(ir_generation_context &) const override;

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        std::u32string _referenced_name;

        expression * _referenced = nullptr;
        mutable const expression * _base = nullptr;

        std::unique_ptr<member_assignment_expression> _assignment_expr;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct member_expression;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<member_access_expression> preanalyze_member_access_expression(precontext & ctx,
        const parser::member_expression & parse,
        scope *,
        std::optional<std::u32string> name);

    inline std::unique_ptr<member_access_expression> make_member_access_expression(
        std::u32string referenced_name,
        type * ref_type,
        scope * lex_scope,
        std::optional<std::u32string> name = std::nullopt)
    {
        return std::make_unique<member_access_expression>(
            lex_scope, std::move(name), std::move(referenced_name), ref_type);
    }
}
}
