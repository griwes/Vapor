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
#include "vapor/analyzer/statements/if.h"
#include "vapor/analyzer/statements/return.h"

using namespace reaver::vapor;
using namespace reaver::vapor::analyzer;

MAYFLY_BEGIN_SUITE("analyzer");
MAYFLY_BEGIN_SUITE("statements");
MAYFLY_BEGIN_SUITE("if");

MAYFLY_ADD_TESTCASE("single branch, true", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    auto current_scope = &s;

    auto ast = parse(U"if (true) { return 1; }", parser::parse_if_statement);

    std::unique_ptr<statement> if_stmt = preanalyze_if_statement(pctx, ast, current_scope);
    auto if_stmt_ptr = if_stmt.get();

    auto analysis_future = if_stmt->analyze(ctx);
    reaver::get(analysis_future);

    MAYFLY_CHECK(if_stmt->get_returns().size() == 1);

    replacements repl;
    cached_results res;
    simplification_context simpl_ctx{ res };

    auto simpl_future = if_stmt->simplify({ simpl_ctx });
    replace_uptr(if_stmt, reaver::get(simpl_future), simpl_ctx);
    MAYFLY_REQUIRE(if_stmt.get() != if_stmt_ptr);

    auto returns = if_stmt->get_returns();
    MAYFLY_REQUIRE(returns.size() == 1);
    MAYFLY_REQUIRE(returns.front() == if_stmt.get());
});

MAYFLY_ADD_TESTCASE("single branch, false", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    auto current_scope = &s;

    auto ast = parse(U"if (false) { return 1; }", parser::parse_if_statement);

    std::unique_ptr<statement> if_stmt = preanalyze_if_statement(pctx, ast, current_scope);
    auto if_stmt_ptr = if_stmt.get();

    auto analysis_future = if_stmt->analyze(ctx);
    reaver::get(analysis_future);

    MAYFLY_CHECK(if_stmt->get_returns().size() == 1);

    replacements repl;
    cached_results res;
    simplification_context simpl_ctx{ res };

    auto simpl_future = if_stmt->simplify({ simpl_ctx });
    replace_uptr(if_stmt, reaver::get(simpl_future), simpl_ctx);
    MAYFLY_REQUIRE(if_stmt.get() != if_stmt_ptr);

    auto returns = if_stmt->get_returns();
    MAYFLY_REQUIRE(returns.size() == 0);
    MAYFLY_REQUIRE(dynamic_cast<null_statement *>(if_stmt.get()));
});

MAYFLY_ADD_TESTCASE("two branches, true", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    auto current_scope = &s;

    auto ast = parse(U"if (true) { return 0; } else { return 1; }", parser::parse_if_statement);

    std::unique_ptr<statement> if_stmt = preanalyze_if_statement(pctx, ast, current_scope);
    auto if_stmt_ptr = if_stmt.get();

    auto analysis_future = if_stmt->analyze(ctx);
    reaver::get(analysis_future);

    MAYFLY_CHECK(if_stmt->get_returns().size() == 2);

    replacements repl;
    cached_results res;
    simplification_context simpl_ctx{ res };

    auto simpl_future = if_stmt->simplify({ simpl_ctx });
    replace_uptr(if_stmt, reaver::get(simpl_future), simpl_ctx);
    MAYFLY_REQUIRE(if_stmt.get() != if_stmt_ptr);

    integer_constant constant{ 0, nullptr, std::nullopt };

    auto returns = if_stmt->get_returns();
    MAYFLY_REQUIRE(returns.size() == 1);
    MAYFLY_REQUIRE(returns.front()->get_returned_expression()->is_equal(&constant));
});

MAYFLY_ADD_TESTCASE("two branches, false", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    auto current_scope = &s;

    auto ast = parse(U"if (false) { return 0; } else { return 1; }", parser::parse_if_statement);

    std::unique_ptr<statement> if_stmt = preanalyze_if_statement(pctx, ast, current_scope);
    auto if_stmt_ptr = if_stmt.get();

    auto analysis_future = if_stmt->analyze(ctx);
    reaver::get(analysis_future);

    MAYFLY_CHECK(if_stmt->get_returns().size() == 2);

    replacements repl;
    cached_results res;
    simplification_context simpl_ctx{ res };

    auto simpl_future = if_stmt->simplify({ simpl_ctx });
    replace_uptr(if_stmt, reaver::get(simpl_future), simpl_ctx);
    MAYFLY_REQUIRE(if_stmt.get() != if_stmt_ptr);

    integer_constant constant{ 1, nullptr, std::nullopt };

    auto returns = if_stmt->get_returns();
    MAYFLY_REQUIRE(returns.size() == 1);
    MAYFLY_REQUIRE(returns.front()->get_returned_expression()->is_equal(&constant));
});

MAYFLY_ADD_TESTCASE("runtime condition", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    auto current_scope = &s;
    test_expression expr{ builtin_types().boolean };
    expr.set_simplified_expression(std::unique_ptr<expression>{ &expr });
    s.init(U"condition", make_symbol(U"condition", &expr));
    s.close();

    auto ast_single = parse(U"if (condition) { return 0; }", parser::parse_if_statement);
    auto ast_double = parse(U"if (condition) { return 0; } else { return 1; }", parser::parse_if_statement);

    auto if_stmt_single = preanalyze_if_statement(pctx, ast_single, current_scope);
    auto if_stmt_double = preanalyze_if_statement(pctx, ast_double, current_scope);

    auto analysis_future_single = if_stmt_single->analyze(ctx);
    auto analysis_future_double = if_stmt_double->analyze(ctx);
    reaver::get(analysis_future_single);
    reaver::get(analysis_future_double);

    replacements repl;
    cached_results res;
    simplification_context simpl_ctx{ res };

    auto simpl_future_single = if_stmt_single->simplify({ simpl_ctx });
    auto simpl_future_double = if_stmt_double->simplify({ simpl_ctx });
    auto simplified_single = reaver::get(simpl_future_single);
    MAYFLY_REQUIRE(simplified_single == if_stmt_single.get());
    auto simplified_double = reaver::get(simpl_future_double);
    MAYFLY_REQUIRE(simplified_double == if_stmt_double.get());
});

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
