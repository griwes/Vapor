/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2017, 2019 Michał "Griwes" Dominiak
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

#include <reaver/mayfly.h>

#include "helpers.h"

using namespace reaver::vapor;
using namespace reaver::vapor::parser;

MAYFLY_BEGIN_SUITE("parser");
MAYFLY_BEGIN_SUITE("postfix_expression");

MAYFLY_ADD_TESTCASE("basic postfix-expression",
    test(UR"(foo;)",
        postfix_expression{ { 0, 3 },
            identifier{ { 0, 3 }, { lexer::token_type::identifier, UR"(foo)", { 0, 3 } } },
            {},
            {} },
        [](auto && ctx) { return parse_postfix_expression(ctx); }));

MAYFLY_ADD_TESTCASE("argumentless",
    test(UR"(foo();)",
        postfix_expression{ { 0, 5 },
            identifier{ { 0, 3 }, { lexer::token_type::identifier, UR"(foo)", { 0, 3 } } },
            lexer::token_type::round_bracket_open,
            {} },
        [](auto && ctx) { return parse_postfix_expression(ctx); }));

MAYFLY_ADD_TESTCASE("chained argumentless",
    test(UR"(foo()();)",
        postfix_expression{ { 0, 7 },
            expression{ { 0, 5 },
                postfix_expression{ { 0, 5 },
                    identifier{ { 0, 3 }, { lexer::token_type::identifier, UR"(foo)", { 0, 3 } } },
                    lexer::token_type::round_bracket_open,
                    {} } },
            lexer::token_type::round_bracket_open,
            {} },
        [](auto && ctx) { return parse_postfix_expression(ctx); }));

MAYFLY_ADD_TESTCASE("one argument",
    test(UR"(foo[1];)",
        postfix_expression{ { 0, 6 },
            identifier{ { 0, 3 }, { lexer::token_type::identifier, UR"(foo)", { 0, 3 } } },
            lexer::token_type::square_bracket_open,
            { { { 4, 5 },
                integer_literal{ { 4, 5 }, { lexer::token_type::integer, UR"(1)", { 4, 5 } }, {} } } } },
        [](auto && ctx) { return parse_postfix_expression(ctx); }));

MAYFLY_ADD_TESTCASE("more arguments",
    test(UR"(foo{ a, b, c };)",
        postfix_expression{ { 0, 14 },
            identifier{ { 0, 3 }, { lexer::token_type::identifier, UR"(foo)", { 0, 3 } } },
            lexer::token_type::curly_bracket_open,
            { { { 5, 6 },
                  postfix_expression{ { 5, 6 },
                      identifier{ { 5, 6 }, { lexer::token_type::identifier, UR"(a)", { 5, 6 } } },
                      {},
                      {} } },
                { { 8, 9 },
                    postfix_expression{ { 8, 9 },
                        identifier{ { 8, 9 }, { lexer::token_type::identifier, UR"(b)", { 8, 9 } } },
                        {},
                        {} } },
                { { 11, 12 },
                    postfix_expression{ { 11, 12 },
                        identifier{ { 11, 12 }, { lexer::token_type::identifier, UR"(c)", { 11, 12 } } },
                        {},
                        {} } } } },
        [](auto && ctx) { return parse_postfix_expression(ctx); }));

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
