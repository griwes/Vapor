/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2019 Michał "Griwes" Dominiak
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

#include "../types/unresolved.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class entity : public expression
    {
    public:
        entity(type *, scope *, std::u32string, std::unique_ptr<expression> = nullptr);
        entity(std::unique_ptr<type>, scope *, std::u32string, std::unique_ptr<expression> = nullptr);
        entity(std::shared_ptr<unresolved_type>,
            scope *,
            std::u32string,
            std::unique_ptr<expression> = nullptr);

        virtual void print(std::ostream &, print_context) const override;

        virtual expression * _get_replacement() override
        {
            return _wrapped ? _wrapped->_get_replacement() : this;
        }

        virtual const expression * _get_replacement() const override
        {
            return _wrapped ? _wrapped->_get_replacement() : this;
        }

        void set_source_hash(std::string hash)
        {
            _source_hash = std::move(hash);
        }

        auto & get_source_hash() const
        {
            return _source_hash;
        }

        void set_timestamp(std::int64_t timestamp)
        {
            _timestamp = timestamp;
        }

        auto get_timestamp() const
        {
            return _timestamp;
        }

        void set_import_name(std::vector<std::string> name)
        {
            _name = std::move(name);
        }

        auto & get_import_name() const
        {
            return _name;
        }

        void add_import_dependency(entity * ent)
        {
            _import_deps.push_back(ent);
        }

        auto & get_import_dependencies() const
        {
            return _import_deps;
        }

        void mark_local()
        {
            _is_local = true;
        }

        bool is_local() const
        {
            return _is_local;
        }

        void save_symbol(symbol * symb)
        {
            _symbol = symb;
        }

        symbol * get_symbol() const
        {
            return _symbol;
        }

        std::vector<codegen::ir::entity> module_codegen_ir(ir_generation_context & ctx) const;
        virtual declaration_ir declaration_codegen_ir(ir_generation_context & ctx) const override;

    private:
        virtual future<> _analyze(analysis_context &) override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override;
        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const override;

        std::optional<std::shared_ptr<unresolved_type>> _unresolved;
        std::optional<std::unique_ptr<type>> _owned;
        std::unique_ptr<expression> _wrapped;

        std::string _source_hash;
        std::int64_t _timestamp;
        std::vector<std::string> _name;
        std::vector<entity *> _import_deps;

        bool _is_local = false;
        symbol * _symbol = nullptr;
    };

    inline std::unique_ptr<entity> make_entity(imported_type imported, scope * lex_scope, std::u32string name)
    {
        return std::get<0>(fmap(imported, [&](auto && imported) {
            return std::make_unique<entity>(std::move(imported), lex_scope, std::move(name));
        }));
    }

    inline std::unique_ptr<entity> make_entity(std::unique_ptr<type> owned,
        scope * lex_scope,
        std::u32string name)
    {
        return std::make_unique<entity>(std::move(owned), lex_scope, std::move(name));
    }

    std::unique_ptr<entity> get_entity(precontext &, const proto::entity &);
}
}
