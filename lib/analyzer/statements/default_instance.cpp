/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019-2020 Michał "Griwes" Dominiak
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
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/typeclass.h"

#include "entity.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<default_instance> preanalyze_default_instance(precontext & ctx,
        const parser::default_instance_definition & parse,
        scope * lex_scope)
    {
        ++ctx.default_instance_definition_count;

        auto instance = preanalyze_instance_literal(ctx, parse.literal, lex_scope, true);

        auto symb = make_symbol(U"", instance.get());
        symb->mark_exported();
        lex_scope->init_unnamed(std::move(symb));

        return std::make_unique<default_instance>(make_node(parse), std::move(instance));
    }

    std::unique_ptr<default_instance> import_default_instance(precontext & ctx, const proto::entity & entity)
    {
        ++ctx.default_instance_definition_count;

        auto type = get_imported_type_ref(ctx, entity.type());

        auto instance = import_typeclass_instance(ctx, type, entity.typeclass_instance(), true);
        auto expression = std::make_unique<typeclass_instance_expression>(
            imported_ast_node(ctx, entity.range()), std::move(instance));

        return std::make_unique<default_instance>(
            imported_ast_node(ctx, entity.range()), std::move(expression));
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
        os << styles::def << ctx << styles::rule_name << "default-instance";
        print_address_range(os, this);
        os << '\n';

        auto inst_ctx = ctx.make_branch(true);
        os << styles::def << inst_ctx << styles::subrule_name << "instance:\n";
        _instance->print(os, inst_ctx.make_branch(true));
    }

    typeclass_instance * default_instance::get_defined_instance() const
    {
        return _instance->get_instance();
    }

    typeclass_instance_expression * default_instance::get_defined_instance_expr() const
    {
        return _instance.get();
    }

    declaration_ir default_instance::declaration_codegen_ir(ir_generation_context & ctx) const
    {
        return _instance->declaration_codegen_ir(ctx);
    }

    future<> default_instance::_analyze(analysis_context & ctx)
    {
        auto instance_analysis = _instance->analyze(ctx);

        return _instance->get_instance_type_future()
            .then([&, rest = std::move(instance_analysis)](auto) {
                ctx.add_default_instance_definition(this);
                return rest;
            })
            .then([&] { _instance->mark_exported(); });
    }

    future<statement *> default_instance::_simplify(recursive_context ctx)
    {
        return _instance->simplify_expr(ctx).then([this, &ctx = ctx.proper](auto && simpl) -> statement * {
            assert(simpl == _instance.get());
            return this;
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
