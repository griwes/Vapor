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

#include "statement.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class typeclass_instance_expression;
    class typeclass_instance;

    class default_instance : public statement
    {
    public:
        default_instance(ast_node parse, std::unique_ptr<typeclass_instance_expression> instance);
        ~default_instance();

        virtual void print(std::ostream & os, print_context ctx) const override;

        typeclass_instance * get_defined_instance() const;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<statement> _clone(replacements & repl) const override;
        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override;

        std::unique_ptr<typeclass_instance_expression> _instance;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct default_instance_definition;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<default_instance> preanalyze_default_instance(precontext & ctx,
        const parser::default_instance_definition & parse,
        scope * lex_scope);
}
}
