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

#include "../expressions/expression.h"
#include "../expressions/expression_ref.h"
#include "../expressions/type.h"
#include "../types/unresolved.h"
#include "instance_context.h"
#include "symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class archetype;

    class parameter : public expression
    {
    public:
        parameter(ast_node parse, scope * lex_scope, std::u32string name, std::unique_ptr<expression> type);
        ~parameter();

        virtual void print(std::ostream & os, print_context ctx) const override;

        expression * get_type_expression() const
        {
            return _type_expression.get();
        }

    private:
        template<typename Self>
        static auto _get_replacement_helper(Self &&);

        virtual expression * _get_replacement() override;
        virtual const expression * _get_replacement() const override;

        virtual future<> _analyze(analysis_context & ctx) override;
        virtual future<expression *> _simplify_expr(recursive_context ctx) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override;
        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;
        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

        std::unique_ptr<expression> _type_expression;

        std::unique_ptr<archetype> _archetype;
    };

    using parameter_list = std::vector<std::unique_ptr<parameter>>;
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct parameter_list;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    parameter_list preanalyze_parameter_list(precontext & ctx,
        const parser::parameter_list & param_list,
        scope * lex_scope,
        std::optional<instance_function_context> = std::nullopt);
}
}
