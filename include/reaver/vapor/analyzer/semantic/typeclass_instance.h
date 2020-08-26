/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019-2020 Michał "Griwes" Dominiak
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

#pragma once

#include "../expressions/expression.h"
#include "instance_context.h"

namespace reaver::vapor::proto
{
struct typeclass_instance;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function_definition;
    class overload_set_expression;
    class refined_overload_set_expression;
    class block;

    class typeclass_instance
    {
    public:
        typeclass_instance(ast_node parse,
            std::unique_ptr<scope> member_scope,
            std::vector<std::u32string> typeclass_name,
            std::vector<std::unique_ptr<expression>> arguments);
        typeclass_instance(ast_node parse, std::unique_ptr<scope> member_scope, imported_type type);

        std::vector<expression *> get_arguments() const;
        std::vector<type *> get_argument_values() const;
        future<> simplify_arguments(analysis_context &);

        std::vector<function_definition *> get_member_function_defs() const;

        void set_type(typeclass_instance_type * type);
        function_definition_handler get_function_definition_handler();
        void import_default_definitions(analysis_context & ctx, bool is_imported = false);

        void mark_exported();
        void mark_imported();
        bool is_imported() const;
        void mark_default();
        bool is_default() const;

        scope * get_scope()
        {
            return _scope.get();
        }

        const scope * get_scope() const
        {
            return _scope.get();
        }

        const auto & typeclass_reference() const
        {
            return _typeclass_reference;
        }

        const typeclass * get_typeclass() const;
        void print(std::ostream & os, print_context ctx, bool print_members = false) const;

        std::optional<ast_node> get_ast_info() const
        {
            return std::make_optional(_node);
        }

        const typeclass_instance_type * get_type() const
        {
            return _type;
        }

        std::unique_ptr<proto::typeclass_instance> generate_interface() const;

    private:
        struct _function_specialization
        {
            std::unique_ptr<function> spec;
            std::unique_ptr<block> function_body;
        };

        ast_node _node;
        std::optional<std::u32string> _name;

        std::unique_ptr<scope> _scope;
        std::variant<std::vector<std::u32string>, imported_type> _typeclass_reference;
        typeclass_instance_type * _type = nullptr;
        bool _exported = false;
        bool _imported = false;
        bool _default = false;

        std::vector<std::unique_ptr<expression>> _arguments;
        std::vector<std::unique_ptr<function_definition>> _member_function_definitions;

        std::vector<_function_specialization> _function_specializations;
        std::vector<std::unique_ptr<refined_overload_set_expression>> _member_overload_set_exprs;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct instance_literal;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_instance> make_typeclass_instance(precontext & ctx,
        const parser::instance_literal & parse,
        scope * lex_scope,
        bool is_default,
        std::optional<std::u32string> canonical_name = std::nullopt);
    std::unique_ptr<typeclass_instance> import_typeclass_instance(precontext & ctx,
        imported_type,
        const proto::typeclass_instance &,
        bool is_default);
}
}
