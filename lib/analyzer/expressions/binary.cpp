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

#include <boost/type_index.hpp>

#include "vapor/analyzer/expressions/binary.h"
#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/overloads.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/parser.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<binary_expression> preanalyze_binary_expression(precontext & ctx,
        const parser::binary_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        return std::make_unique<binary_expression>(make_node(parse),
            parse.op,
            preanalyze_expression(ctx, parse.lhs, lex_scope),
            preanalyze_expression(ctx, parse.rhs, lex_scope),
            lex_scope,
            std::move(name));
    }

    binary_expression::binary_expression(ast_node parse,
        lexer::token op,
        std::unique_ptr<expression> lhs,
        std::unique_ptr<expression> rhs,
        scope * lex_scope,
        std::optional<std::u32string> name)
        : expression{ lex_scope, std::move(name) }, _op{ op }, _lhs{ std::move(lhs) }, _rhs{ std::move(rhs) }
    {
        _set_ast_info(parse);
    }

    void binary_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "binary-expression";
        print_address_range(os, this);
        os << '\n';

        auto type_ctx = ctx.make_branch(false);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        get_type()->print(os, type_ctx.make_branch(true));

        auto lhs_ctx = ctx.make_branch(false);
        os << styles::def << lhs_ctx << styles::subrule_name << "lhs:\n";
        _lhs->print(os, lhs_ctx.make_branch(true));

        auto operator_ctx = ctx.make_branch(false);
        os << styles::def << operator_ctx << styles::subrule_name << "operator: " << styles::string_value
           << lexer::token_types[+_op.type] << '\n';

        auto rhs_ctx = ctx.make_branch(false);
        os << styles::def << rhs_ctx << styles::subrule_name << "rhs:\n";
        _rhs->print(os, rhs_ctx.make_branch(true));

        auto call_expr_ctx = ctx.make_branch(true);
        os << styles::def << call_expr_ctx << styles::subrule_name << "resolved expression:\n";
        _call_expression->print(os, call_expr_ctx.make_branch(true));
    }

    future<> binary_expression::_analyze(analysis_context & ctx)
    {
        auto expr_ctx = get_context();
        expr_ctx.push_back(this);

        _lhs->set_context(expr_ctx);
        _rhs->set_context(expr_ctx);

        return when_all(_lhs->analyze(ctx), _rhs->analyze(ctx))
            .then([&](auto) {
                return resolve_overload(ctx,
                    this->get_ast_info().value().range,
                    get_scope(),
                    get_name(),
                    _lhs.get(),
                    _rhs.get(),
                    _op.type);
            })
            .then([&](std::unique_ptr<expression> call_expr) {
                if (auto call_expr_downcasted = call_expr->as<call_expression>())
                {
                    call_expr_downcasted->set_ast_info(get_ast_info().value());
                }
                _call_expression = std::move(call_expr);
                return _call_expression->analyze(ctx);
            })
            .then([&] { this->_set_type(_call_expression->get_type()); });
    }

    std::unique_ptr<expression> binary_expression::_clone_expr(replacements & repl) const
    {
        return repl.claim(_call_expression.get());
    }

    future<expression *> binary_expression::_simplify_expr(recursive_context ctx)
    {
        replacements repl;
        auto clone = _clone_expr(repl).release();

        return clone->simplify_expr(ctx).then([ctx, clone](auto && simplified) {
            if (simplified)
            {
                ctx.proper.keep_alive(clone);
                return simplified;
            }
            return clone;
        });
    }

    statement_ir binary_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        // TODO: there should be a different order of evaluation for right-associative and left-associative
        // (...probably)
        return _call_expression->codegen_ir(ctx);
    }

    constant_init_ir binary_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }
}
}
