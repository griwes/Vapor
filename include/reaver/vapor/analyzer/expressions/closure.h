/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2017 Michał "Griwes" Dominiak
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

#include "../function.h"
#include "../scope.h"
#include "../semantic/parameter_list.h"
#include "../statements/block.h"
#include "../statements/statement.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class closure : public expression
    {
    public:
        closure(const parser::lambda_expression & parse, scope * lex_scope);

        const auto & parse() const
        {
            return _parse;
        }

        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual declaration_ir declaration_codegen_ir(ir_generation_context &) const override;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr_with_replacement(replacements &) const override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;

        const parser::lambda_expression & _parse;
        parameter_list _parameter_list;

        optional<std::unique_ptr<expression>> _return_type;
        std::unique_ptr<scope> _scope;
        std::unique_ptr<block> _body;
        std::unique_ptr<type> _type;
    };

    inline std::unique_ptr<closure> preanalyze_closure(const parser::lambda_expression & parse, scope * lex_scope)
    {
        return std::make_unique<closure>(parse, lex_scope);
    }
}
}
