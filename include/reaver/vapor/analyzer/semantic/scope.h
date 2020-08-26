/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014, 2016-2020 Michał "Griwes" Dominiak
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
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <reaver/exception.h>
#include <reaver/optional.h>

#include "../../utf.h"
#include "../ir_context.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class failed_lookup : public exception
    {
    public:
        failed_lookup(const std::u32string & n) : exception{ logger::error }, name{ n }
        {
            *this << "failed scope lookup for `" << utf8(name) << "`.";
        }

        std::u32string name;
    };

    class symbol;

    enum class scope_kind
    {
        global,
        module,
        type,
        local
    };

    class scope
    {
        struct _key
        {
        };

    public:
        scope(scope_kind kind = scope_kind::global) : _scope_kind{ kind }
        {
        }

    public:
        scope(_key,
            scope * parent_scope,
            scope_kind kind,
            bool is_shadowing_boundary,
            bool is_special = false,
            const scope * other = nullptr);
        ~scope();

        void close();

        scope * parent() const
        {
            return _parent;
        }

        std::unique_ptr<scope> clone_for_module(std::u32string name);
        std::unique_ptr<scope> clone_for_type(std::u32string name);
        std::unique_ptr<scope> _clone_for_default_instance();
        std::unique_ptr<scope> clone_for_local();
        scope * clone_for_decl();

        symbol * get(const std::u32string & name) const;
        std::optional<symbol *> try_get(const std::u32string & name) const;

        symbol * init(const std::u32string & name, std::unique_ptr<symbol> symb);
        symbol * init_unnamed(std::unique_ptr<symbol> symb);

        template<typename F>
        auto get_or_init(const std::u32string & name, F init)
        {
            if (auto symb = try_get(name))
            {
                return symb.value();
            }

            auto init_v = init();
            auto ret = init_v.get();
            _symbols_in_order.push_back(init_v.get());
            _symbols.emplace(name, std::move(init_v));
            return ret;
        }

        symbol * resolve(const std::u32string & name) const;
        std::vector<symbol *> declared_symbols() const;

        void set_name(std::u32string name)
        {
            assert(_name.empty());
            _name = std::move(name);
        }

        std::u32string_view get_name() const
        {
            assert(!_name.empty());
            return _name;
        }

        scope_kind get_kind() const
        {
            return _scope_kind;
        }

        std::u32string_view get_scoped_name() const
        {
            if (!_full_name)
            {
                if (_is_special)
                {
                    _full_name = _name + U".";
                }

                else if (!_name.empty())
                {
                    _full_name = std::u32string{ _parent ? _parent->get_scoped_name() : U"" } + _name + U".";
                }

                else
                {
                    _full_name = U"";
                }
            }

            return _full_name.value();
        }

        std::u32string get_entity_name() const
        {
            if (_is_special)
            {
                return _name;
            }

            return !_name.empty() ? std::u32string{ _parent ? _parent->get_scoped_name() : U"" } + _name
                                  : U"";
        }

        const auto & symbols_in_order() const
        {
            assert(_is_closed);
            return _symbols_in_order;
        }

        void keep_alive()
        {
            assert(_parent);
            auto inserted = _parent->_keepalive.emplace(this).second;
            assert(inserted);
        }

        void mark_global()
        {
            _global = this;
        }

    private:
        std::u32string _name;
        mutable std::optional<std::u32string> _full_name;

        scope * _parent = nullptr;
        scope * _global = nullptr;
        std::unordered_set<std::unique_ptr<scope>> _keepalive;
        std::unordered_map<std::u32string, std::unique_ptr<symbol>> _symbols;
        std::vector<std::unique_ptr<symbol>> _unnamed_symbols;
        std::vector<symbol *> _symbols_in_order;
        mutable std::unordered_map<std::u32string, symbol *> _resolve_cache;
        const scope_kind _scope_kind = scope_kind::global;
        const bool _is_shadowing_boundary = false;
        bool _is_closed = false;
        bool _is_special = false;
    };

    void initialize_global_scope(scope * lex_scope, std::vector<std::shared_ptr<void>> & keepalive_list);
}
}
