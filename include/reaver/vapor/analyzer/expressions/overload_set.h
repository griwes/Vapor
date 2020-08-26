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

#include "../precontext.h"
#include "../semantic/overload_set.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function_declaration;
    class function_definition;

    class overload_set_expression_base : public expression
    {
    public:
        using expression::expression;

        virtual void print(std::ostream & os, print_context) const override
        {
            assert(0);
        }

        std::vector<function *> get_overloads() const
        {
            return get_overload_set()->get_overloads();
        }

        virtual overload_set_base * get_overload_set() const = 0;
        virtual std::unordered_set<expression *> get_associated_entities() const override;
    };

    class overload_set_expression : public overload_set_expression_base
    {
    public:
        overload_set_expression(scope * lex_scope, std::u32string name);
        overload_set_expression(std::shared_ptr<overload_set> t, scope * lex_scope, std::u32string name);

        // TODO: =default once I've thrown the analysis future nonsense out of this hierarchy
        // and into analysis_context
        overload_set_expression(const overload_set_expression & other,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : overload_set_expression_base{ lex_scope, std::move(name) }, _oset{ other._oset }
        {
            _set_type(_oset->get_type());
        }

        virtual declaration_ir declaration_codegen_ir(ir_generation_context & ctx) const override;

        virtual void mark_exported() override
        {
            _oset->get_type()->mark_exported();
        }

        virtual overload_set * get_overload_set() const override
        {
            return _oset.get();
        }

        virtual bool is_constant() const override;

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;
        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

        std::shared_ptr<overload_set> _oset;
    };

    class refined_overload_set_expression : public overload_set_expression_base
    {
    public:
        refined_overload_set_expression(overload_set * base, scope * lex_scope, std::u32string name);
        refined_overload_set_expression(std::shared_ptr<refined_overload_set> oset,
            scope * lex_scope,
            std::u32string name);

        // TODO: =default once I've thrown the analysis future nonsense out of this hierarchy
        // and into analysis_context
        refined_overload_set_expression(const refined_overload_set_expression & other,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : overload_set_expression_base{ lex_scope, std::move(name) }, _oset{ other._oset }
        {
            _set_type(_oset->get_type());
        }

        virtual refined_overload_set * get_overload_set() const override
        {
            return _oset.get();
        }

        virtual bool is_constant() const override;
        virtual function * get_vtable_entry(std::size_t id) const override;

        virtual void mark_exported() override
        {
            _is_exported = true;
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;
        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

        std::shared_ptr<refined_overload_set> _oset;
        bool _is_exported = false;
    };

    std::unique_ptr<overload_set_expression> create_overload_set(scope * lex_scope, std::u32string name);
    std::unique_ptr<refined_overload_set_expression> create_refined_overload_set(scope * lex_scope,
        std::u32string name,
        overload_set * base);
    std::unique_ptr<overload_set_expression_base> get_overload_set(scope * lex_scope, std::u32string name);
    std::unique_ptr<overload_set_expression> get_overload_set_special(scope * lex_scope, std::u32string name);
    std::unique_ptr<refined_overload_set_expression> get_refined_overload_set(scope * lex_scope,
        std::u32string name,
        overload_set * base);
}
}

namespace reaver::vapor::proto
{
class user_defined_reference;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class unresolved_overload_set_expression : public overload_set_expression_base
    {
    public:
        unresolved_overload_set_expression(precontext & ctx,
            const proto::user_defined_reference * udr,
            scope * lex_scope);

        virtual overload_set_base * get_overload_set() const override;
        virtual bool is_constant() const override;
        virtual function * get_vtable_entry(std::size_t id) const override;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        precontext * _ctx;
        synthesized_udr _udr;
        std::unique_ptr<overload_set_expression_base> _resolved;
    };

    std::unique_ptr<unresolved_overload_set_expression> make_unresolved_overload_set_expression(
        precontext & ctx,
        const proto::user_defined_reference * udr,
        scope * lex_scope);
}
}
