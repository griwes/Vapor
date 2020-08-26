/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/struct_literal.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/declaration.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<struct_literal> preanalyze_struct_literal(precontext & ctx,
        const parser::struct_literal & parse,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name)
    {
        return std::make_unique<struct_literal>(
            make_node(parse), make_struct_type(ctx, parse, lex_scope, canonical_name));
    }

    struct_literal::struct_literal(ast_node parse, std::unique_ptr<struct_type> type)
        : expression{ builtin_types().type,
              type->get_scope()->parent(),
              std::u32string{ type->get_scope()->get_name() } },
          _type{ std::move(type) }
    {
        _set_ast_info(parse);
    }

    void struct_literal::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "struct-literal";
        print_address_range(os, this);
        os << '\n';

        auto type_ctx = ctx.make_branch(_type->get_data_member_decls().empty());
        os << styles::def << type_ctx << styles::subrule_name << "defined type:\n";
        _type->print(os, type_ctx.make_branch(true));

        if (!_type->get_data_member_decls().empty())
        {
            auto data_members_ctx = ctx.make_branch(true);
            os << styles::def << data_members_ctx << styles::subrule_name << "data member definitions:\n";

            std::size_t idx = 0;
            for (auto && member : _type->get_data_member_decls())
            {
                member->print(
                    os, data_members_ctx.make_branch(++idx == _type->get_data_member_decls().size()));
            }
        }
    }

    void struct_literal::mark_exported()
    {
        _type->mark_exported();
    }

    future<> struct_literal::_analyze(analysis_context & ctx)
    {
        return when_all(fmap(_type->get_data_member_decls(), [&](auto && member) {
            return member->analyze(ctx);
        })).then([this] { _type->generate_constructors(); });
    }

    std::unique_ptr<expression> struct_literal::_clone_expr(replacements & repl) const
    {
        assert(0);
    }

    future<expression *> struct_literal::_simplify_expr(recursive_context ctx)
    {
        return make_ready_future<expression *>(this);
    }

    statement_ir struct_literal::_codegen_ir(ir_generation_context & ctx) const
    {
        assert(0);
    }

    constant_init_ir struct_literal::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    std::unique_ptr<google::protobuf::Message> struct_literal::_generate_interface() const
    {
        return _type->get_expression()->_do_generate_interface();
    }
}
}
