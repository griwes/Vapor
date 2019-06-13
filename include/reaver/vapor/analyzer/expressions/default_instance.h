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

#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class default_instance_expression : public expression
    {
    public:
        default_instance_expression(ast_node parse, std::unique_ptr<expression> instance_type_expr);

        virtual void print(std::ostream &, print_context) const override;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual future<expression *> _simplify_expr(recursive_context ctx) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        std::unique_ptr<expression> _instance_type_expr;
        std::unique_ptr<expression> _instance_expr;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct default_instance_expression;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<default_instance_expression> preanalyze_default_instance_expression(precontext & ctx,
        const parser::default_instance_expression & parse,
        scope * lex_scope);
}
}

