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

#include <memory>

#include "../../print_helpers.h"
#include "../types/unresolved.h"
#include "context.h"
#include "scope.h"

namespace reaver::vapor::proto
{
class typeclass;
class user_defined_reference;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class parameter;
    class function_declaration;
    class typeclass_instance_type;

    class typeclass
    {
    public:
        typeclass(ast_node parse,
            std::unique_ptr<scope> member_scope,
            std::vector<std::unique_ptr<parameter>> parameters,
            std::vector<std::unique_ptr<function_declaration>> member_function_decls);
        typeclass(ast_node parse,
            std::unique_ptr<scope> member_scope,
            std::vector<std::unique_ptr<parameter>> parameters,
            std::vector<std::unique_ptr<expression>> keepalive);

        ~typeclass();

        std::vector<parameter *> get_parameters() const;
        std::vector<expression *> get_parameter_expressions() const;

        std::vector<function_declaration *> get_member_function_decls() const
        {
            return fmap(_member_function_declarations, [](auto && ptr) { return ptr.get(); });
        }

        auto get_ast_info() const
        {
            return std::make_optional(_parse);
        }

        scope * get_scope()
        {
            return _scope.get();
        }

        const scope * get_scope() const
        {
            return _scope.get();
        }

        future<typeclass_instance_type *> type_for(analysis_context & ctx, const std::vector<type *> & args);

        void print(std::ostream & os, print_context ctx, bool print_members = false) const;

        std::unique_ptr<proto::typeclass> generate_interface() const;
        std::unique_ptr<proto::user_defined_reference> generate_interface_reference() const;

    private:
        ast_node _parse;

        std::unique_ptr<scope> _scope;
        std::vector<std::unique_ptr<parameter>> _parameters;
        std::vector<std::unique_ptr<expression>> _keepalive;

        std::vector<std::unique_ptr<function_declaration>> _member_function_declarations;
        std::vector<function *> _member_functions;

        struct instance_information
        {
            std::unique_ptr<typeclass_instance_type> instance;
            std::optional<future<typeclass_instance_type *>> analysis_future;
        };

        std::unordered_map<std::vector<type *>, instance_information, type_list_hash, type_list_compare>
            _instance_types;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    class typeclass_literal;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;
    class typeclass;

    std::unique_ptr<typeclass> make_typeclass(precontext & ctx,
        const parser::typeclass_literal & parse,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name = std::nullopt);
    std::unique_ptr<typeclass> import_typeclass(precontext & ctx, const proto::typeclass &);
}
}
