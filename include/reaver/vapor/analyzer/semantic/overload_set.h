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

#include "../types/overload_set.h"
#include "scope.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class overload_set_expression;
    class function;
    class function_declaration;
    struct imported_function;

    class overload_set_base
    {
    public:
        virtual ~overload_set_base() = default;

        void add_function(function * fn);
        void add_function(std::unique_ptr<imported_function> fn);

        virtual std::vector<function *> get_overloads() const = 0;

        virtual overload_set_type * get_type() const = 0;

        virtual scope * get_scope() const
        {
            return get_type()->get_scope();
        }

        virtual std::u32string get_name() const
        {
            return get_scope()->get_entity_name();
        }

    protected:
        std::vector<function *> _functions;
        // shared so that this is destructible without knowing the definition
        std::vector<std::shared_ptr<imported_function>> _imported_functions;
    };

    class overload_set final : public overload_set_base
    {
    public:
        overload_set(std::unique_ptr<scope> lex_scope);
        ~overload_set() = default;

        virtual std::vector<function *> get_overloads() const override;
        virtual overload_set_type * get_type() const override;

        void print(std::ostream & os, print_context ctx, bool print_members = false) const;

    private:
        std::unique_ptr<overload_set_type> _type;
    };

    class refined_overload_set final : public overload_set_base
    {
    public:
        refined_overload_set(scope * lex_scope, overload_set * base);

        virtual std::vector<function *> get_overloads() const override;
        virtual overload_set_type * get_type() const override;

        virtual scope * get_scope() const override
        {
            return _refined_scope.get();
        }

        overload_set * get_base() const
        {
            return _base;
        }

        void resolve_overrides();
        void verify_overrides() const;
        function * get_vtable_entry(std::size_t vtable_id) const;

        void print(std::ostream & os, print_context ctx, bool print_members = false) const;

    private:
        overload_set * _base;
        std::unique_ptr<scope> _refined_scope;

        std::unordered_map<std::size_t, function *> _vtable_entries;
    };
}
}

namespace reaver::vapor::proto
{
class overload_set_type;
class overload_set;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<overload_set> import_overload_set(precontext &, const proto::overload_set_type &);
}
}
