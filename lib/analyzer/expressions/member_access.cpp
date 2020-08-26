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

#include "vapor/analyzer/expressions/member_access.h"
#include "vapor/analyzer/expressions/binary.h"
#include "vapor/analyzer/expressions/postfix.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/parser/member_expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<member_access_expression> preanalyze_member_access_expression(precontext &,
        const parser::member_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        return std::make_unique<member_access_expression>(
            make_node(parse), lex_scope, std::move(name), parse.member_name.value.string);
    }

    void member_access_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "member-access-expression";
        if (get_ast_info())
        {
            print_address_range(os, this);
        }
        else
        {
            os << styles::def << " @ " << styles::address << this << styles::def << ": ";
        }
        os << styles::string_value << ' ' << utf8(_referenced_name) << '\n';

        auto type_ctx = ctx.make_branch(true);
        os << styles::def << type_ctx << styles::subrule_name << "referenced member type:\n";
        get_type()->print(os, type_ctx.make_branch(true));
    }

    future<> member_access_expression::_analyze(analysis_context & ctx)
    {
        auto & expr_ctx = get_context();
        assert(!expr_ctx.empty());

        auto last_postfix = std::find_if(
            expr_ctx.rbegin(), expr_ctx.rend(), [](auto && ctx_expr) { return ctx_expr.index() == 0; });
        assert(last_postfix != expr_ctx.rend());

        // this is actually conceptually "last_postfix + 1" (if last_postfix was a normal iterator)
        // reverse iterators are weird
        auto next = last_postfix.base();

        if (next != expr_ctx.end())
        {
            auto top_level = std::get<binary_expression *>(*next);

            // if this is on the LHS of a binary expression at the top level
            // of a postfix expression (in the future: also tuple-expression)
            // then this is an id part of a member assignment expression
            // and hence we need to do a Special Thing
            if (top_level->get_lhs() == this && top_level->get_operator() == lexer::token_type::assign)
            {
                _assignment_expr = make_member_assignment_expression(_referenced_name);
                _set_type(_assignment_expr->get_type());
                return make_ready_future();
            }
        }

        return std::get<postfix_expression *>(*last_postfix)
            ->get_base_expression(ctx)
            .then([&](auto && base) { this->set_base_expression(base); });
    }

    void member_access_expression::set_base_expression(expression * base)
    {
        _base = base;

        auto referenced_type = base->get_type()->get_member_type(_referenced_name);
        assert(referenced_type && "no member found for whatever reason");

        _referenced = base->get_member(_referenced_name);
        this->_set_type(referenced_type);
    }

    statement_ir member_access_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        _base = _base ? _base : ctx.get_current_base();

        auto base_variable_value = _base->codegen_ir(ctx).back().result;
        auto base_variable = std::get<std::shared_ptr<codegen::ir::variable>>(base_variable_value);

        auto retvar = codegen::ir::make_variable(
            _base->get_type()->get_member_type(_referenced_name)->codegen_type(ctx));

        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::member_access_instruction>() },
            { base_variable, codegen::ir::label{ _referenced_name } },
            retvar } };
    }

    constant_init_ir member_access_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    bool member_access_expression::_invalidate_ir(ir_generation_context & ctx) const
    {
        return ctx.get_current_base() != _base;
    }
}
}
