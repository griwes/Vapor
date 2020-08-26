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
#include "vapor/analyzer/expressions/struct_literal.h"
#include "vapor/analyzer/expressions/type.h"

using namespace reaver::vapor;
using namespace reaver::vapor::analyzer;

MAYFLY_BEGIN_SUITE("analyzer");
MAYFLY_BEGIN_SUITE("expressions");
MAYFLY_BEGIN_SUITE("struct");

MAYFLY_ADD_TESTCASE("empty struct", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    std::vector<std::shared_ptr<void>> keepalive;
    initialize_global_scope(&s, keepalive);
    auto current_scope = &s;

    auto ast = parse(U"struct {}", parser::parse_struct_literal);

    auto struct_lit = preanalyze_struct_literal(pctx, ast, current_scope, U"placeholder");

    reaver::get(struct_lit->analyze(ctx));

    auto type_var = struct_lit->as<type_expression>();
    MAYFLY_REQUIRE(type_var);

    auto type = type_var->get_value();
    auto struct_type = dynamic_cast<class struct_type *>(type);
    MAYFLY_REQUIRE(struct_type);

    MAYFLY_CHECK(struct_type->get_data_member_decls().size() == 0);
    MAYFLY_CHECK(struct_type->get_data_members().size() == 0);
    MAYFLY_CHECK(reaver::get(struct_type->get_constructor({}))->parameters().size() == 0);
    MAYFLY_CHECK(reaver::get(struct_type->get_candidates(lexer::token_type::curly_bracket_open))
                     .front()
                     ->parameters()
                     .size()
        == 1);
});

MAYFLY_ADD_TESTCASE("struct with members", [] {
    config::compiler_options opts{ std::make_unique<config::language_options>() };
    analysis_context ctx;
    precontext pctx{ opts, ctx };

    scope s;
    std::vector<std::shared_ptr<void>> keepalive;
    initialize_global_scope(&s, keepalive);
    auto current_scope = &s;

    auto ast = parse(UR"code(
            struct
            {
                let i = 1;
                let j : int;
                let k : int = 2;
            }
        )code",
        parser::parse_struct_literal);

    auto struct_lit = preanalyze_struct_literal(pctx, ast, current_scope, U"placeholder");

    reaver::get(struct_lit->analyze(ctx));

    auto type_var = struct_lit->as<type_expression>();
    MAYFLY_REQUIRE(type_var);

    auto type = type_var->get_value();
    auto struct_type = dynamic_cast<class struct_type *>(type);
    MAYFLY_REQUIRE(struct_type);

    MAYFLY_CHECK(struct_type->get_data_member_decls().size() == 3);
    MAYFLY_CHECK(struct_type->get_data_members().size() == 3);
    MAYFLY_CHECK(reaver::get(struct_type->get_constructor({}))->parameters().size() == 3);
    MAYFLY_CHECK(reaver::get(struct_type->get_candidates(lexer::token_type::curly_bracket_open))
                     .front()
                     ->parameters()
                     .size()
        == 4);
});

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
