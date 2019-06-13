/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/default_instance.h"

#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<default_instance_expression> preanalyze_default_instance_expression(precontext & ctx,
        const parser::default_instance_expression & parse,
        scope * lex_scope)
    {
        return std::make_unique<default_instance_expression>(
            make_node(parse), preanalyze_expression(ctx, parse.instance_type, lex_scope));
    }

    default_instance_expression::default_instance_expression(ast_node parse,
        std::unique_ptr<expression> instance_type_expr)
        : _instance_type_expr{ std::move(instance_type_expr) }
    {
        _set_ast_info(parse);
    }

    void default_instance_expression::print(std::ostream &, print_context) const
    {
        assert(0);
    }

    future<> default_instance_expression::_analyze(analysis_context & ctx)
    {
        return _instance_type_expr->analyze(ctx)
            .then([&] {
                auto type_expr = _instance_type_expr->as<type_expression>();
                assert(type_expr);
                auto type = type_expr->get_value();
                _set_type(type);

                auto instance_type = dynamic_cast<typeclass_instance_type *>(type);
                assert(instance_type);

                return instance_type->get_default_instance_expr(ctx);
            })
            .then([&](auto &&) { assert(0); });
    }

    future<expression *> default_instance_expression::_simplify_expr(recursive_context ctx)
    {
        assert(0);
    }

    std::unique_ptr<expression> default_instance_expression::_clone_expr(replacements &) const
    {
        assert(0);
    }

    statement_ir default_instance_expression::_codegen_ir(ir_generation_context &) const
    {
        assert(0);
    }

    constant_init_ir default_instance_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }
}
}
