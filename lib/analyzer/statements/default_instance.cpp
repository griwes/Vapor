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

#include "vapor/analyzer/statements/default_instance.h"

#include "vapor/analyzer/expressions/typeclass_instance.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<default_instance> preanalyze_default_instance(precontext & ctx,
        const parser::default_instance_definition & parse,
        scope * lex_scope)
    {
        ++ctx.default_instance_definition_count;

        auto instance = preanalyze_instance_literal(ctx, parse.literal, lex_scope);

        return std::make_unique<default_instance>(make_node(parse), std::move(instance));
    }

    default_instance::default_instance(ast_node parse,
        std::unique_ptr<typeclass_instance_expression> instance)
        : _instance{ std::move(instance) }
    {
        _set_ast_info(parse);
    }

    default_instance::~default_instance() = default;

    void default_instance::print(std::ostream & os, print_context ctx) const
    {
        assert(0);
    }

    typeclass_instance * default_instance::get_defined_instance() const
    {
        return _instance->get_instance();
    }

    future<> default_instance::_analyze(analysis_context & ctx)
    {
        auto instance_analysis = _instance->analyze(ctx);

        return _instance->get_instance_type_future().then([&, rest = std::move(instance_analysis)](auto) {
            ctx.add_default_instance_definition(this);
            return rest;
        });
    }

    std::unique_ptr<statement> default_instance::_clone(replacements & repl) const
    {
        assert(0);
    }

    statement_ir default_instance::_codegen_ir(ir_generation_context & ctx) const
    {
        assert(0);
    }

}
}
