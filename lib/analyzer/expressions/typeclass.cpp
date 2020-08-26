/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/semantic/typeclass.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/typeclass.h"

#include "expressions/typeclass.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_expression> preanalyze_typeclass_literal(precontext & ctx,
        const parser::typeclass_literal & parse,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name)
    {
        return std::make_unique<typeclass_expression>(
            make_node(parse), make_typeclass(ctx, parse, lex_scope, std::move(canonical_name)));
    }

    typeclass_expression::typeclass_expression(ast_node parse, std::unique_ptr<typeclass> tc)
        : expression{ tc->get_scope()->parent(), std::u32string{ tc->get_scope()->get_name() } },
          _typeclass{ std::move(tc) }
    {
        _set_ast_info(parse);
    }

    typeclass_expression::~typeclass_expression() = default;

    void typeclass_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "typeclass-literal";
        print_address_range(os, this);
        os << '\n';

        auto tc_ctx = ctx.make_branch(false);
        os << styles::def << tc_ctx << styles::subrule_name << "defined typeclass:\n";
        _typeclass->print(os, tc_ctx.make_branch(true), true);

        auto params_ctx = ctx.make_branch(_typeclass->get_member_function_decls().empty());
        os << styles::def << params_ctx << styles::subrule_name << "parameters:\n";

        std::size_t idx = 0;
        for (auto && param : _typeclass->get_parameter_expressions())
        {
            param->print(os, params_ctx.make_branch(++idx == _typeclass->get_parameter_expressions().size()));
        }

        if (_typeclass->get_member_function_decls().size())
        {
            auto decl_ctx = ctx.make_branch(true);
            os << styles::def << decl_ctx << styles::subrule_name << "member function declarations:\n";

            std::size_t idx = 0;
            for (auto && member : _typeclass->get_member_function_decls())
            {
                member->print(
                    os, decl_ctx.make_branch(++idx == _typeclass->get_member_function_decls().size()));
            }
        }
    }

    future<> typeclass_expression::_analyze(analysis_context & ctx)
    {
        return when_all(
            fmap(_typeclass->get_parameters(), [&](auto && param) { return param->analyze(ctx); }))
            .then([&] {
                auto param_types = fmap(_typeclass->get_parameters(), [](auto && param) {
                    auto ret = param->get_type();
                    assert(ret->is_meta());
                    return ret;
                });
                _set_type(ctx.get_typeclass_type(param_types));
            })
            .then([&] {
                return when_all(fmap(_typeclass->get_member_function_decls(),
                    [&](auto && decl) { return decl->analyze(ctx); }));
            })
            .then([&] {
                // analyze possible unresolved types in function signatures
                return when_all(fmap(_typeclass->get_scope()->symbols_in_order(), [&](symbol * symb) {
                    auto && oset = symb->get_expression()->as<overload_set_expression>();
                    if (oset)
                    {
                        return when_all(fmap(oset->get_overloads(), [&](function * fn) {
                            return fn->return_type_expression()->analyze(ctx).then([fn, &ctx] {
                                return when_all(fmap(
                                    fn->parameters(), [&](auto && param) { return param->analyze(ctx); }));
                            });
                        }));
                    }

                    return make_ready_future();
                }));
            });
    }

    std::unique_ptr<expression> typeclass_expression::_clone_expr(replacements & repl) const
    {
        assert(0);
    }

    future<expression *> typeclass_expression::_simplify_expr(recursive_context ctx)
    {
        return when_all(fmap(_typeclass->get_member_function_decls(), [&](auto && decl) {
            return decl->simplify(ctx);
        })).then([&](auto &&) -> expression * { return this; });
    }

    statement_ir typeclass_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        assert(0);
    }

    constant_init_ir typeclass_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    std::unique_ptr<google::protobuf::Message> typeclass_expression::_generate_interface() const
    {
        return _typeclass->generate_interface();
    }
}
}
