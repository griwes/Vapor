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

#include <reaver/future_get.h>
#include <reaver/mayfly.h>

#include "../helpers.h"
#include "vapor/analyzer/expressions/integer.h"
#include "vapor/analyzer/expressions/struct.h"

using namespace reaver::vapor;
using namespace reaver::vapor::analyzer;

MAYFLY_BEGIN_SUITE("analyzer");
MAYFLY_BEGIN_SUITE("expressions");
MAYFLY_BEGIN_SUITE("struct value");

MAYFLY_ADD_TESTCASE("constant construction and replacement", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    std::vector<std::shared_ptr<void>> keepalive;
    initialize_global_scope(&s, keepalive);
    auto current_scope = &s;

    auto type_ast = parse(UR"code(
            struct foo
            {
                let i : int;
                let j : int;
            };
        )code",
        parser::parse_struct_declaration);
    auto struct_decl = preanalyze_declaration(pctx, type_ast, current_scope);

    auto expression_ast = parse(UR"code(
            let bar = foo{ 1, 2 };
        )code",
        [](auto && ctx) { return parser::parse_declaration(ctx); });
    std::unique_ptr<statement> declaration = preanalyze_declaration(pctx, expression_ast, current_scope);
    current_scope->close();

    reaver::get(struct_decl->analyze(ctx));
    reaver::get(declaration->analyze(ctx));

    cached_results res;
    simplification_context simpl_ctx{ res };
    do
    {
        simpl_ctx.~simplification_context();
        new (&simpl_ctx) simplification_context(res);

        replace_uptr(declaration, reaver::get(declaration->simplify({ simpl_ctx })), simpl_ctx);
        reaver::get(current_scope->get(U"bar")->simplify({ simpl_ctx }));
    } while (simpl_ctx.did_something_happen());

    auto type_expr = struct_decl->declared_symbol()->get_expression()->as<type_expression>();
    MAYFLY_CHECK(type_expr);

    auto type = type_expr->get_value();
    auto struct_type = dynamic_cast<class struct_type *>(type);
    MAYFLY_REQUIRE(struct_type);

    auto data_members = struct_type->get_data_members();
    MAYFLY_CHECK(data_members.size() == 2);

    integer_constant const_one{ 1, nullptr, std::nullopt };
    integer_constant const_two{ 2, nullptr, std::nullopt };

    auto struct_expr = current_scope->get(U"bar")->get_expression();
    MAYFLY_CHECK(struct_expr->get_type() == struct_type);
    MAYFLY_REQUIRE(struct_expr->is_constant());

    MAYFLY_CHECK(struct_expr->get_member(data_members[0]->get_name().value())->is_equal(&const_one));
    MAYFLY_CHECK(struct_expr->get_member(data_members[1]->get_name().value())->is_equal(&const_two));

    // replacement

    auto replacement_ast = parse(UR"code(
            bar{ 3 }
        )code",
        [](auto && ctx) { return parser::parse_expression(ctx); });

    auto replaced_expr = preanalyze_expression(pctx, replacement_ast, current_scope);

    reaver::get(replaced_expr->analyze(ctx));
    do
    {
        simpl_ctx.~simplification_context();
        new (&simpl_ctx) simplification_context(res);

        replace_uptr(replaced_expr, reaver::get(replaced_expr->simplify_expr({ simpl_ctx })), simpl_ctx);
    } while (simpl_ctx.did_something_happen());

    integer_constant const_three{ 3, nullptr, std::nullopt };

    MAYFLY_CHECK(replaced_expr->get_type() == struct_type);
    MAYFLY_REQUIRE(replaced_expr->is_constant());

    MAYFLY_CHECK(replaced_expr->get_member(data_members[0]->get_name().value())->is_equal(&const_three));
    MAYFLY_CHECK(replaced_expr->get_member(data_members[1]->get_name().value())->is_equal(&const_two));

    // designated replacement

    auto designated_repl_ast = parse(UR"code(
                bar{ .j = 3 }
            )code",
        [](auto && ctx) { return parser::parse_expression(ctx); });

    auto designated_repl_expr = preanalyze_expression(pctx, designated_repl_ast, current_scope);

    reaver::get(designated_repl_expr->analyze(ctx));
    do
    {
        simpl_ctx.~simplification_context();
        new (&simpl_ctx) simplification_context(res);

        replace_uptr(
            designated_repl_expr, reaver::get(designated_repl_expr->simplify_expr({ simpl_ctx })), simpl_ctx);
    } while (simpl_ctx.did_something_happen());

    MAYFLY_CHECK(designated_repl_expr->get_type() == struct_type);
    MAYFLY_REQUIRE(designated_repl_expr->is_constant());

    MAYFLY_CHECK(designated_repl_expr->get_member(data_members[0]->get_name().value())->is_equal(&const_one));
    MAYFLY_CHECK(
        designated_repl_expr->get_member(data_members[1]->get_name().value())->is_equal(&const_three));
});

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
