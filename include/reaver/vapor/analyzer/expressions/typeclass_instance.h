/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/instance_context.h"
#include "vapor/analyzer/types/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class expression_list;
    class function_definition;
    class typeclass_instance;

    class typeclass_instance_expression : public expression
    {
    public:
        using late_preanalysis_type = reaver::unique_function<void(function_definition_handler)>;

        typeclass_instance_expression(ast_node parse,
            scope * original_scope,
            std::vector<std::u32string> name_segments,
            std::vector<std::unique_ptr<expression>> arguments,
            late_preanalysis_type late_pre);

        virtual ~typeclass_instance_expression() override;

        virtual void print(std::ostream & os, print_context ctx) const override;

    private:
        virtual future<> _analyze(analysis_context & ctx) override;
        virtual future<expression *> _simplify_expr(recursive_context ctx) override;
        virtual std::unique_ptr<expression> _clone_expr_with_replacement(replacements & repl) const override;
        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override;

        scope * _original_scope;

        std::vector<std::u32string> _typeclass_name;
        std::vector<std::unique_ptr<expression>> _arguments;

        late_preanalysis_type _late_preanalysis;

        std::shared_ptr<typeclass_instance> _instance;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct instance_literal;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_instance_expression> preanalyze_instance_literal(precontext & ctx,
        const parser::instance_literal & tpl,
        scope * lex_scope);
}
}
