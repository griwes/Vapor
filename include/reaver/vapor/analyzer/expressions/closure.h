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

#include <memory>
#include <numeric>

#include "../semantic/function.h"
#include "../semantic/parameter_list.h"
#include "../semantic/scope.h"
#include "../statements/block.h"
#include "../statements/statement.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class closure : public expression
    {
    public:
        closure(ast_node parse,
            std::unique_ptr<scope> sc,
            parameter_list params,
            std::unique_ptr<block> body,
            std::optional<std::unique_ptr<expression>> return_type,
            scope * lex_scope,
            std::optional<std::u32string> name);

        virtual void print(std::ostream & os, print_context ctx) const override;

        virtual declaration_ir declaration_codegen_ir(ir_generation_context &) const override;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        parameter_list _parameter_list;

        std::optional<std::unique_ptr<expression>> _return_type;
        std::unique_ptr<scope> _scope;
        std::unique_ptr<block> _body;
        std::unique_ptr<type> _type;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct lambda_expression;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontex;
    std::unique_ptr<closure> preanalyze_closure(precontext & ctx,
        const parser::lambda_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name);
}
}
