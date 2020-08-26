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

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class scope;
    class function;

    class postfix_expression : public expression
    {
    public:
        postfix_expression(ast_node parse,
            scope * lex_scope,
            std::optional<std::u32string> name,
            std::unique_ptr<expression> base,
            std::optional<lexer::token_type> mod,
            std::vector<std::unique_ptr<expression>> arguments,
            std::optional<std::u32string> accessed_member);

        virtual void print(std::ostream & os, print_context ctx) const override;

        future<expression *> get_base_expression(analysis_context & ctx) const
        {
            return _base_expr->analyze(ctx).then([&] { return _base_expr.get(); });
        }

    private:
        template<typename Self>
        static auto _get_replacement_helper(Self &&);

        virtual expression * _get_replacement() override;
        virtual const expression * _get_replacement() const override;

        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        virtual bool _is_equal(const expression * rhs) const override
        {
            if (_referenced_expression)
            {
                return _referenced_expression.value()->is_equal(rhs);
            }

            if (_call_expression)
            {
                return _call_expression->is_equal(rhs);
            }

            return _base_expr->is_equal(rhs);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        std::unique_ptr<expression> _base_expr;
        std::optional<lexer::token_type> _modifier;
        std::vector<std::unique_ptr<expression>> _arguments;
        std::unique_ptr<expression> _call_expression;

        std::optional<std::u32string> _accessed_member;
        std::optional<expression *> _referenced_expression;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct postfix_expression;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<postfix_expression> preanalyze_postfix_expression(precontext & ctx,
        const parser::postfix_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name);
}
}
