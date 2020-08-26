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
#include "vapor/analyzer/semantic/typeclass.h"
#include "vapor/analyzer/types/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class typeclass_expression : public expression
    {
    public:
        typeclass_expression(ast_node parse, std::unique_ptr<typeclass> type);
        ~typeclass_expression();

        virtual void print(std::ostream & os, print_context ctx) const override;

        typeclass * get_typeclass() const
        {
            return _typeclass.get();
        }

    private:
        virtual future<> _analyze(analysis_context & ctx) override;
        virtual future<expression *> _simplify_expr(recursive_context ctx) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override;
        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;
        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

        std::unique_ptr<typeclass> _typeclass;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct typeclass_literal;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_expression> preanalyze_typeclass_literal(precontext & ctx,
        const parser::typeclass_literal & tpl,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name = std::nullopt);
}
}
