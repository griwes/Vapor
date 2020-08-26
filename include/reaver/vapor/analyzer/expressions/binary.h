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
    class function;

    class binary_expression : public expression
    {
    public:
        binary_expression(ast_node parse,
            lexer::token op,
            std::unique_ptr<expression> lhs,
            std::unique_ptr<expression> rhs,
            scope * lex_scope,
            std::optional<std::u32string> name);

        virtual void print(std::ostream & os, print_context ctx) const override;

        const expression * get_lhs() const
        {
            return _lhs.get();
        }

        const expression * get_rhs() const
        {
            return _rhs.get();
        }

        lexer::token_type get_operator() const
        {
            return _op.type;
        }

    private:
        virtual expression * _get_replacement() override
        {
            return _call_expression->_get_replacement();
        }

        virtual const expression * _get_replacement() const override
        {
            return _call_expression->_get_replacement();
        }

        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const override;

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        lexer::token _op;
        std::unique_ptr<expression> _lhs;
        std::unique_ptr<expression> _rhs;
        std::unique_ptr<expression> _call_expression;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct binary_expression;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;
    std::unique_ptr<binary_expression> preanalyze_binary_expression(precontext & ctx,
        const parser::binary_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name);
}
}
