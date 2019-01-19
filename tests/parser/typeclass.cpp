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

#include <reaver/mayfly.h>

#include "helpers.h"

using namespace reaver::vapor;
using namespace reaver::vapor::parser;

MAYFLY_BEGIN_SUITE("parser");
MAYFLY_BEGIN_SUITE("typeclass");

MAYFLY_BEGIN_SUITE("definition");

MAYFLY_ADD_TESTCASE("empty literal",
    test(UR"(typeclass(t : type) {})",
        typeclass_literal{ { 0, 22 },
            { { 10, 18 },
                { parameter{
                    { 10, 18 },
                    { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                    std::make_optional<expression>(
                        { { 14, 18 }, postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                } } },
            {} },
        &parse_typeclass_literal));

MAYFLY_ADD_TESTCASE("empty declaration",
    test(UR"(typeclass a(t : type) {})",
        declaration{ { 0, 24 },
            std::nullopt,
            { { 10, 11 }, { lexer::token_type::identifier, UR"(a)", { 10, 11 } } },
            std::nullopt,
            std::make_optional<expression>({ { 0, 24 },
                typeclass_literal{ { 0, 24 },
                    { { 10, 18 },
                        { parameter{
                            { 10, 18 },
                            { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                            std::make_optional<expression>({ { 14, 18 },
                                postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                        } } },
                    {} } }) },
        &parse_typeclass_definition));

MAYFLY_ADD_TESTCASE("literal with a function declaration",
    test(UR"(typeclass(t : type) { function foo() -> int; })",
        typeclass_literal{ { 0, 46 },
            { { 10, 18 },
                { parameter{
                    { 10, 18 },
                    { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                    std::make_optional<expression>(
                        { { 14, 18 }, postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                } } },
            { function_declaration{ { 22, 43 },
                std::nullopt,
                { { 31, 34 }, { lexer::token_type::identifier, UR"(foo)", { 31, 34 } } },
                std::nullopt,
                std::make_optional(expression{ { 40, 43 },
                    postfix_expression{ { 40, 43 }, { identifier{ { 40, 43 }, { lexer::token_type::identifier, UR"(int)", { 40, 43 } } } } } }) } } },
        &parse_typeclass_literal));

MAYFLY_ADD_TESTCASE("declaration with a function declaration",
    test(UR"(typeclass a(t : type) { function foo() -> int; })",
        declaration{ { 0, 48 },
            std::nullopt,
            { { 10, 11 }, { lexer::token_type::identifier, UR"(a)", { 10, 11 } } },
            std::nullopt,
            std::make_optional<expression>({ { 0, 48 },
                typeclass_literal{ { 0, 48 },
                    { { 10, 18 },
                        { parameter{
                            { 10, 18 },
                            { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                            std::make_optional<expression>({ { 14, 18 },
                                postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                        } } },
                    { function_declaration{ { 24, 45 },
                        std::nullopt,
                        { { 33, 36 }, { lexer::token_type::identifier, UR"(foo)", { 33, 36 } } },
                        std::nullopt,
                        std::make_optional(expression{ { 42, 45 },
                            postfix_expression{ { 42, 45 },
                                { identifier{ { 42, 45 }, { lexer::token_type::identifier, UR"(int)", { 42, 45 } } } } } }) } } } }) },
        &parse_typeclass_definition));

MAYFLY_ADD_TESTCASE("literal with a function definition",
    test(UR"(typeclass(t : type) { function foo() -> int { return 1; } })",
        typeclass_literal{ { 0, 59 },
            { { 10, 18 },
                { parameter{
                    { 10, 18 },
                    { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                    std::make_optional<expression>(
                        { { 14, 18 }, postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                } } },
            { function_definition{ { 22, 57 },
                function_declaration{ { 22, 43 },
                    std::nullopt,
                    { { 31, 34 }, { lexer::token_type::identifier, UR"(foo)", { 31, 34 } } },
                    std::nullopt,
                    std::make_optional(expression{ { 40, 43 },
                        postfix_expression{ { 40, 43 }, { identifier{ { 40, 43 }, { lexer::token_type::identifier, UR"(int)", { 40, 43 } } } } } }) },
                block{ { 44, 57 },
                    { statement{ { 46, 55 },
                        return_expression{ { 46, 54 }, { { 53, 54 }, integer_literal{ { 53, 54 }, { lexer::token_type::integer, UR"(1)", { 53, 54 } } } } } } },
                    std::nullopt } } } },
        &parse_typeclass_literal));

MAYFLY_ADD_TESTCASE("declaration with a function definition",
    test(UR"(typeclass a(t : type) { function foo() -> int { return 1; } })",
        declaration{ { 0, 61 },
            std::nullopt,
            { { 10, 11 }, { lexer::token_type::identifier, UR"(a)", { 10, 11 } } },
            std::nullopt,
            std::make_optional<expression>({ { 0, 61 },
                { typeclass_literal{ { 0, 61 },
                    { { 10, 18 },
                        { parameter{
                            { 10, 18 },
                            { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                            std::make_optional<expression>({ { 14, 18 },
                                postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                        } } },
                    { function_definition{ { 24, 59 },
                        function_declaration{ { 24, 45 },
                            std::nullopt,
                            { { 33, 36 }, { lexer::token_type::identifier, UR"(foo)", { 33, 36 } } },
                            std::nullopt,
                            std::make_optional(expression{ { 42, 45 },
                                postfix_expression{ { 42, 45 }, { identifier{ { 42, 45 }, { lexer::token_type::identifier, UR"(int)", { 42, 45 } } } } } }) },
                        block{ { 46, 59 },
                            { statement{ { 48, 57 },
                                return_expression{ { 48, 56 },
                                    { { 55, 56 }, integer_literal{ { 55, 56 }, { lexer::token_type::integer, UR"(1)", { 55, 56 } } } } } } },
                            std::nullopt } } } } } }) },
        &parse_typeclass_definition));

MAYFLY_ADD_TESTCASE("literal with both a function definition and a function declaration",
    test(UR"(typeclass(t : type) { function foo() -> int; function bar() -> int { return 1; } })",
        typeclass_literal{ { 0, 82 },
            { { 10, 18 },
                { parameter{
                    { 10, 18 },
                    { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                    std::make_optional<expression>(
                        { { 14, 18 }, postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                } } },
            { function_declaration{ { 22, 43 },
                  std::nullopt,
                  { { 31, 34 }, { lexer::token_type::identifier, UR"(foo)", { 31, 34 } } },
                  std::nullopt,
                  std::make_optional(expression{ { 40, 43 },
                      postfix_expression{ { 40, 43 }, { identifier{ { 40, 43 }, { lexer::token_type::identifier, UR"(int)", { 40, 43 } } } } } }) },
                function_definition{ { 45, 80 },
                    function_declaration{ { 45, 66 },
                        std::nullopt,
                        { { 54, 57 }, { lexer::token_type::identifier, UR"(bar)", { 54, 57 } } },
                        std::nullopt,
                        std::make_optional(expression{ { 63, 66 },
                            postfix_expression{ { 63, 66 }, { identifier{ { 63, 66 }, { lexer::token_type::identifier, UR"(int)", { 63, 66 } } } } } }) },
                    block{ { 67, 80 },
                        { statement{ { 69, 78 },
                            return_expression{ { 69, 77 },
                                { { 76, 77 }, integer_literal{ { 76, 77 }, { lexer::token_type::integer, UR"(1)", { 76, 77 } } } } } } },
                        std::nullopt } } } },
        &parse_typeclass_literal));

MAYFLY_ADD_TESTCASE("declaration with both a function definition and a function declaration",
    test(UR"(typeclass a(t : type) { function foo() -> int; function bar() -> int { return 1; } })",
        declaration{ { 0, 84 },
            std::nullopt,
            { { 10, 11 }, { lexer::token_type::identifier, UR"(a)", { 10, 11 } } },
            std::nullopt,
            std::make_optional<expression>({ { 0, 84 },
                { typeclass_literal{ { 0, 84 },
                    { { 10, 18 },
                        { parameter{
                            { 10, 18 },
                            { { 10, 11 }, { lexer::token_type::identifier, UR"(t)", { 10, 11 } } },
                            std::make_optional<expression>({ { 14, 18 },
                                postfix_expression{ { 14, 18 }, identifier{ { 14, 18 }, { lexer::token_type::identifier, UR"(type)", { 14, 18 } } } } }),
                        } } },
                    { function_declaration{ { 24, 45 },
                          std::nullopt,
                          { { 33, 36 }, { lexer::token_type::identifier, UR"(foo)", { 33, 36 } } },
                          std::nullopt,
                          std::make_optional(expression{ { 42, 45 },
                              postfix_expression{ { 42, 45 }, { identifier{ { 42, 45 }, { lexer::token_type::identifier, UR"(int)", { 42, 45 } } } } } }) },
                        function_definition{ { 47, 82 },
                            function_declaration{ { 47, 68 },
                                std::nullopt,
                                { { 56, 59 }, { lexer::token_type::identifier, UR"(bar)", { 56, 59 } } },
                                std::nullopt,
                                std::make_optional(expression{ { 65, 68 },
                                    postfix_expression{ { 65, 68 },
                                        { identifier{ { 65, 68 }, { lexer::token_type::identifier, UR"(int)", { 65, 68 } } } } } }) },
                            block{ { 69, 82 },
                                { statement{ { 71, 80 },
                                    return_expression{ { 71, 79 },
                                        { { 78, 79 }, integer_literal{ { 78, 79 }, { lexer::token_type::integer, UR"(1)", { 78, 79 } } } } } } },
                                std::nullopt } } } } } }) },
        &parse_typeclass_definition));

MAYFLY_END_SUITE;

MAYFLY_BEGIN_SUITE("instance");

MAYFLY_ADD_TESTCASE("default instance with no members",
    test(UR"(default instance a(int) {})",
        default_instance_definition{ { 0, 26 },
            instance_literal{ { 8, 26 },
                { { 17, 18 }, { { { 17, 18 }, { lexer::token_type::identifier, UR"(a)", { 17, 18 } } } } },
                { { 19, 22 },
                    { expression{ { 19, 22 },
                        postfix_expression{ { 19, 22 }, { identifier{ { 19, 22 }, { lexer::token_type::identifier, UR"(int)", { 19, 22 } } } } } } } },
                {} } },
        &parse_default_instance));

MAYFLY_ADD_TESTCASE("instance literal with no members",
    test(UR"(instance a(int) {})",
        instance_literal{ { 0, 18 },
            { { 9, 10 }, { { { 9, 10 }, { lexer::token_type::identifier, UR"(a)", { 9, 10 } } } } },
            { { 11, 14 },
                { expression{ { 11, 14 },
                    postfix_expression{ { 11, 14 }, { identifier{ { 11, 14 }, { lexer::token_type::identifier, UR"(int)", { 11, 14 } } } } } } } },
            {} },
        &parse_instance_literal));

MAYFLY_ADD_TESTCASE("default instance with definitions",
    test(UR"(default instance a(int) { function foo(arg : int) {} function bar(arg) {} })",
        default_instance_definition{ { 0, 75 },
            instance_literal{ { 8, 75 },
                { { 17, 18 }, { { { 17, 18 }, { lexer::token_type::identifier, UR"(a)", { 17, 18 } } } } },
                { { 19, 22 },
                    { expression{ { 19, 22 },
                        postfix_expression{ { 19, 22 }, { identifier{ { 19, 22 }, { lexer::token_type::identifier, UR"(int)", { 19, 22 } } } } } } } },
                { function_definition{ { 26, 52 },
                      function_declaration{ { 26, 49 },
                          std::nullopt,
                          { { 35, 38 }, { lexer::token_type::identifier, UR"(foo)", { 35, 38 } } },
                          parameter_list{ { 39, 48 },
                              { parameter{ { 39, 48 },
                                  { { 39, 42 }, { lexer::token_type::identifier, UR"(arg)", { 39, 42 } } },
                                  std::make_optional(expression{ { 45, 48 },
                                      postfix_expression{ { 45, 48 },
                                          identifier{ { 45, 48 }, { lexer::token_type::identifier, UR"(int)", { 45, 48 } } } } }) } } },
                          std::nullopt },
                      block{ { 50, 52 }, {}, std::nullopt } },
                    function_definition{ { 53, 73 },
                        function_declaration{ { 53, 70 },
                            std::nullopt,
                            { { 62, 65 }, { lexer::token_type::identifier, UR"(bar)", { 62, 65 } } },
                            parameter_list{ { 66, 69 },
                                { parameter{ { 66, 69 }, { { 66, 69 }, { lexer::token_type::identifier, UR"(arg)", { 66, 69 } } }, std::nullopt } } },
                            std::nullopt },
                        block{ { 71, 73 }, {}, std::nullopt } } } } },
        &parse_default_instance));

MAYFLY_ADD_TESTCASE("instance literal with definitions",
    test(UR"(instance a(int) { function foo(arg : int) {} function bar(arg) {} })",
        instance_literal{ { 0, 67 },
            { { 9, 10 }, { { { 9, 10 }, { lexer::token_type::identifier, UR"(a)", { 9, 10 } } } } },
            { { 11, 14 },
                { expression{ { 11, 14 },
                    postfix_expression{ { 11, 14 }, { identifier{ { 11, 14 }, { lexer::token_type::identifier, UR"(int)", { 11, 14 } } } } } } } },
            { function_definition{ { 18, 44 },
                  function_declaration{ { 18, 41 },
                      std::nullopt,
                      { { 27, 30 }, { lexer::token_type::identifier, UR"(foo)", { 27, 30 } } },
                      parameter_list{ { 31, 40 },
                          { parameter{ { 31, 40 },
                              { { 31, 34 }, { lexer::token_type::identifier, UR"(arg)", { 31, 34 } } },
                              std::make_optional(expression{ { 37, 40 },
                                  postfix_expression{ { 37, 40 }, identifier{ { 37, 40 }, { lexer::token_type::identifier, UR"(int)", { 37, 40 } } } } }) } } },
                      std::nullopt },
                  block{ { 42, 44 }, {}, std::nullopt } },
                function_definition{ { 45, 65 },
                    function_declaration{ { 45, 62 },
                        std::nullopt,
                        { { 54, 57 }, { lexer::token_type::identifier, UR"(bar)", { 54, 57 } } },
                        parameter_list{ { 58, 61 },
                            { parameter{ { 58, 61 }, { { 58, 61 }, { lexer::token_type::identifier, UR"(arg)", { 58, 61 } } }, std::nullopt } } },
                        std::nullopt },
                    block{ { 63, 65 }, {}, std::nullopt } } } },
        &parse_instance_literal));

MAYFLY_END_SUITE;

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
