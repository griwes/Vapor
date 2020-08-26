/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2020 Michał "Griwes" Dominiak
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

#include "../../range.h"
#include "../semantic/function.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class call_expression : public expression
    {
    public:
        call_expression(function * fun,
            std::unique_ptr<expression> vtable_arg,
            std::vector<expression *> args,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : expression{ lex_scope, std::move(name) },
              _function{ fun },
              _vtable_arg{ std::move(vtable_arg) },
              _args{ std::move(args) }
        {
        }

        virtual void print(std::ostream &, print_context ctx) const override;

        void replace_with(std::unique_ptr<expression> expr)
        {
            // assert(!_replacement_expr);

            _replacement_expr = std::move(expr);
            if (auto * replacement_call_expr = _replacement_expr->as<call_expression>())
            {
                replacement_call_expr->_set_ast_info(get_ast_info().value());
            }
        }

        range_type get_range() const
        {
            return get_ast_info().value().range;
        }

        void set_ast_info(ast_node node)
        {
            _set_ast_info(node);
        }

    private:
        virtual expression * _get_replacement() override
        {
            return _replacement_expr ? _replacement_expr->_get_replacement() : this;
        }

        virtual const expression * _get_replacement() const override
        {
            return _replacement_expr ? _replacement_expr->_get_replacement() : this;
        }

        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override;

    protected:
        virtual future<expression *> _simplify_expr(recursive_context) override;

    private:
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const override;

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

    protected:
        function * _function = nullptr;
        std::unique_ptr<expression> _vtable_arg = nullptr;
        std::unique_ptr<expression> _replacement_expr;

    private:
        std::vector<expression *> _args;
        std::unique_ptr<expression> _cloned_type_expr;
    };

    class owning_call_expression : public call_expression
    {
    public:
        owning_call_expression(function * fun,
            std::unique_ptr<expression> vtable_arg,
            std::vector<std::unique_ptr<expression>> args,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : call_expression{ fun,
                  std::move(vtable_arg),
                  fmap(args, [](auto && arg) { return arg.get(); }),
                  lex_scope,
                  std::move(name) },
              _var_exprs{ std::move(args) }
        {
        }

    private:
        virtual future<expression *> _simplify_expr(recursive_context) override;

        std::vector<std::unique_ptr<expression>> _var_exprs;
    };

    inline auto make_call_expression(function * fun,
        expression * vtable_arg,
        std::vector<expression *> args,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        replacements repl;
        return std::make_unique<call_expression>(
            fun, fun->vtable_slot() ? repl.claim(vtable_arg) : nullptr, args, lex_scope, std::move(name));
    }
}
}
