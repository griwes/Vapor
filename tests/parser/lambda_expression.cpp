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
MAYFLY_BEGIN_SUITE("lambda_expression");

MAYFLY_ADD_TESTCASE("no parameters, deduced type, simple body",
    test(UR"(λ => constant;)",
        lambda_expression{ { 0, 13 },
            {},
            {},
            {},
            block{ { 2, 13 },
                {},
                std::make_optional(expression{ { 5, 13 },
                    postfix_expression{ { 5, 13 },
                        { identifier{ { 5, 13 },
                            { lexer::token_type::identifier, UR"(constant)", { 5, 13 } } } },
                        {},
                        {} } }) } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("no parameters, explicit type, simple body",
    test(UR"(λ -> int => constant;)",
        lambda_expression{ { 0, 20 },
            {},
            {},
            std::make_optional(expression{ { 5, 8 },
                postfix_expression{ { 5, 8 },
                    { identifier{
                        { 5, 8 },
                        { lexer::token_type::identifier, UR"(int)", { 5, 8 } },
                    } },
                    {},
                    {} } }),
            block{ { 9, 20 },
                {},
                { std::make_optional(expression{ { 12, 20 },
                    postfix_expression{ { 12, 20 },
                        { identifier{ { 12, 20 },
                            { lexer::token_type::identifier, UR"(constant)", { 12, 20 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("no parameters, deduced type, regular body",
    test(UR"(λ { => constant })",
        lambda_expression{ { 0, 17 },
            {},
            {},
            {},
            block{ { 2, 17 },
                {},
                { std::make_optional(expression{ { 7, 15 },
                    postfix_expression{ { 7, 15 },
                        { identifier{ { 7, 15 },
                            { lexer::token_type::identifier, UR"(constant)", { 7, 15 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("no parameters, explicit type, regular body",
    test(UR"(λ -> int { => constant })",
        lambda_expression{ { 0, 24 },
            {},
            {},
            std::make_optional(expression{ { 5, 8 },
                postfix_expression{ { 5, 8 },
                    { identifier{
                        { 5, 8 },
                        { lexer::token_type::identifier, UR"(int)", { 5, 8 } },
                    } },
                    {},
                    {} } }),
            block{ { 9, 24 },
                {},
                { std::make_optional(expression{ { 14, 22 },
                    postfix_expression{ { 14, 22 },
                        { identifier{ { 14, 22 },
                            { lexer::token_type::identifier, UR"(constant)", { 14, 22 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("one parameter, deduced type, simple body",
    test(UR"(λ(x : int) => constant;)",
        lambda_expression{ { 0, 22 },
            {},
            std::make_optional(parameter_list{ { 2, 9 },
                { parameter{ { 2, 9 },
                    { { 2, 3 }, { lexer::token_type::identifier, UR"(x)", { 2, 3 } } },
                    expression{ { 6, 9 },
                        postfix_expression{ { 6, 9 },
                            identifier{ { 6, 9 }, { lexer::token_type::identifier, UR"(int)", { 6, 9 } } },
                            {},
                            {} } } } } }),
            {},
            block{ { 11, 22 },
                {},
                { std::make_optional(expression{ { 14, 22 },
                    postfix_expression{ { 14, 22 },
                        { identifier{ { 14, 22 },
                            { lexer::token_type::identifier, UR"(constant)", { 14, 22 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("one parameter, explicit type, simple body",
    test(UR"(λ(x : int) -> int => constant;)",
        lambda_expression{ { 0, 29 },
            {},
            std::make_optional(parameter_list{ { 2, 9 },
                { parameter{ { 2, 9 },
                    { { 2, 3 }, { lexer::token_type::identifier, UR"(x)", { 2, 3 } } },
                    expression{ { 6, 9 },
                        postfix_expression{ { 6, 9 },
                            identifier{ { 6, 9 }, { lexer::token_type::identifier, UR"(int)", { 6, 9 } } },
                            {},
                            {} } } } } }),
            std::make_optional(expression{ { 14, 17 },
                postfix_expression{ { 14, 17 },
                    { identifier{
                        { 14, 17 },
                        { lexer::token_type::identifier, UR"(int)", { 14, 17 } },
                    } },
                    {},
                    {} } }),
            block{ { 18, 29 },
                {},
                { std::make_optional(expression{ { 21, 29 },
                    postfix_expression{ { 21, 29 },
                        { identifier{ { 21, 29 },
                            { lexer::token_type::identifier, UR"(constant)", { 21, 29 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("two parameters, deduced type, simple body",
    test(UR"(λ(x : int, y : bool) => constant;)",
        lambda_expression{ { 0, 32 },
            {},
            std::make_optional(parameter_list{ { 2, 19 },
                { parameter{ { 2, 9 },
                      { { 2, 3 }, { lexer::token_type::identifier, UR"(x)", { 2, 3 } } },
                      expression{ { 6, 9 },
                          postfix_expression{ { 6, 9 },
                              identifier{ { 6, 9 }, { lexer::token_type::identifier, UR"(int)", { 6, 9 } } },
                              {},
                              {} } } },
                    parameter{ { 11, 19 },
                        { { 11, 12 }, { lexer::token_type::identifier, UR"(y)", { 11, 12 } } },
                        expression{ { 15, 19 },
                            postfix_expression{ { 15, 19 },
                                identifier{ { 15, 19 },
                                    { lexer::token_type::identifier, UR"(bool)", { 15, 19 } } },
                                {},
                                {} } } } } }),
            {},
            block{ { 21, 32 },
                {},
                { std::make_optional(expression{ { 24, 32 },
                    postfix_expression{ { 24, 32 },
                        { identifier{ { 24, 32 },
                            { lexer::token_type::identifier, UR"(constant)", { 24, 32 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_ADD_TESTCASE("two parameters, explicit type, simple body",
    test(UR"(λ(x : int, y : bool) -> int => constant;)",
        lambda_expression{ { 0, 39 },
            {},
            std::make_optional(parameter_list{ { 2, 19 },
                { parameter{ { 2, 9 },
                      { { 2, 3 }, { lexer::token_type::identifier, UR"(x)", { 2, 3 } } },
                      expression{ { 6, 9 },
                          postfix_expression{ { 6, 9 },
                              identifier{ { 6, 9 }, { lexer::token_type::identifier, UR"(int)", { 6, 9 } } },
                              {},
                              {} } } },
                    parameter{ { 11, 19 },
                        { { 11, 12 }, { lexer::token_type::identifier, UR"(y)", { 11, 12 } } },
                        expression{ { 15, 19 },
                            postfix_expression{ { 15, 19 },
                                identifier{ { 15, 19 },
                                    { lexer::token_type::identifier, UR"(bool)", { 15, 19 } } },
                                {},
                                {} } } } } }),
            std::make_optional(expression{ { 24, 27 },
                postfix_expression{ { 24, 27 },
                    { identifier{
                        { 24, 27 },
                        { lexer::token_type::identifier, UR"(int)", { 24, 27 } },
                    } },
                    {},
                    {} } }),
            block{ { 28, 39 },
                {},
                { std::make_optional(expression{ { 31, 39 },
                    postfix_expression{ { 31, 39 },
                        { identifier{ { 31, 39 },
                            { lexer::token_type::identifier, UR"(constant)", { 31, 39 } } } },
                        {},
                        {} } }) } } },
        &parse_lambda_expression));

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
