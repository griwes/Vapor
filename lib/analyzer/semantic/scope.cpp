/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/semantic/scope.h"

#include <reaver/future_get.h>

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/expressions/function.h"
#include "vapor/analyzer/expressions/integer.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/sized_integer.h"
#include "vapor/analyzer/types/typeclass_instance.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unordered_set<std::u32string> reserved_identifiers = { U"type",
        U"bool",
        U"int",
        U"sized_int",
        U"default" };

    scope::scope(_key,
        scope * parent_scope,
        scope_kind kind,
        bool is_shadowing_boundary,
        bool is_special,
        const scope * other)
        : _parent{ parent_scope },
          _global{ _parent ? _parent->_global : nullptr },
          _scope_kind{ kind },
          _is_shadowing_boundary{ is_shadowing_boundary },
          _is_special{ is_special }
    {
    }

    scope::~scope()
    {
        close();
    }

    void scope::close()
    {
        if (_is_closed)
        {
            return;
        }

        _is_closed = true;

        if (!_is_shadowing_boundary && _parent)
        {
            _parent->close();
        }
    }

    scope * scope::clone_for_decl()
    {
        if (_scope_kind == scope_kind::local)
        {
            return new scope{ _key{}, this, _scope_kind, false };
        }

        return this;
    }

    std::unique_ptr<scope> scope::clone_for_module(std::u32string name)
    {
        auto ret = std::make_unique<scope>(_key{}, this, scope_kind::module, true);
        ret->set_name(std::move(name));
        return ret;
    }

    std::unique_ptr<scope> scope::clone_for_type(std::u32string name)
    {
        auto ret = std::make_unique<scope>(_key{}, this, scope_kind::type, true);
        ret->set_name(std::move(name));
        return ret;
    }

    std::unique_ptr<scope> scope::_clone_for_default_instance()
    {
        return std::make_unique<scope>(_key{}, this, scope_kind::type, true, true);
    }

    std::unique_ptr<scope> scope::clone_for_local()
    {
        return std::make_unique<scope>(_key{}, this, scope_kind::local, true);
    }

    symbol * scope::init(const std::u32string & name, std::unique_ptr<symbol> symb)
    {
        if (reserved_identifiers.count(name) && _global != this)
        {
            assert(0);
        }

        assert(!_is_closed);

        assert(name == symb->get_name());

        for (auto scope = this; scope; scope = scope->_parent)
        {
            if (scope->_symbols.find(name) != scope->_symbols.end())
            {
                return nullptr;
            }

            if (scope->_is_shadowing_boundary)
            {
                break;
            }
        }

        _symbols_in_order.push_back(symb.get());
        return _symbols.emplace(name, std::move(symb)).first->second.get();
    }

    symbol * scope::init_unnamed(std::unique_ptr<symbol> symb)
    {
        assert(!_is_closed);
        assert(symb->get_name() == U"");

        auto ret = symb.get();
        _unnamed_symbols.push_back(std::move(symb));
        _symbols_in_order.push_back(ret);

        return ret;
    }

    symbol * scope::get(const std::u32string & name) const
    {
        auto symb = try_get(name);
        if (!symb)
        {
            throw failed_lookup{ name };
        }
        return symb.value();
    }

    std::optional<symbol *> scope::try_get(const std::u32string & name) const
    {
        auto it = _symbols.find(name);
        if (it == _symbols.end() || it->second->is_hidden())
        {
            return std::nullopt;
        }

        return std::make_optional(it->second.get());
    }

    symbol * scope::resolve(const std::u32string & name) const
    {
        {
            if (reserved_identifiers.count(name))
            {
                return _global->get(name);
            }
        }

        auto it = _resolve_cache.find(name);
        if (it != _resolve_cache.end())
        {
            return it->second;
        }

        auto scope = this;

        while (scope)
        {
            auto symb = scope->try_get(name);

            if (symb && !symb.value()->is_hidden())
            {
                _resolve_cache.emplace(name, symb.value());
                return symb.value();
            }

            scope = scope->_parent;
        }

        throw failed_lookup(name);
    }

    std::vector<symbol *> scope::declared_symbols() const
    {
        assert(_is_closed);

        std::vector<symbol *> ret;
        ret.reserve(_symbols.size() + _unnamed_symbols.size());
        for (auto && [_, symb] : _symbols)
        {
            ret.push_back(symb.get());
        }
        for (auto && symb : _unnamed_symbols)
        {
            ret.push_back(symb.get());
        }
        return ret;
    }

    void initialize_global_scope(scope * lex_scope, std::vector<std::shared_ptr<void>> & keepalive_list)
    {
        lex_scope->mark_global();

        auto integer_type_expr = builtin_types().integer->get_expression();
        auto boolean_type_expr = builtin_types().boolean->get_expression();
        auto type_type_expr = builtin_types().type->get_expression();

        auto add_symbol = [&](auto name, auto && expr) { lex_scope->init(name, make_symbol(name, expr)); };

        add_symbol(U"int", integer_type_expr);
        add_symbol(U"bool", boolean_type_expr);
        add_symbol(U"type", type_type_expr);

        auto sized_int = make_function("sized_int");
        sized_int->set_return_type(type_type_expr);
        sized_int->set_parameters(
            unique_expr_list(make_runtime_value(builtin_types().integer, nullptr, std::nullopt)));

        sized_int->add_analysis_hook(
            [](analysis_context & ctx, call_expression * expr, std::vector<expression *> args) {
                assert(args.size() == 2 && args[1]->is_constant());
                auto int_expr = args[1]->as<integer_constant>();
                assert(int_expr);
                auto size = int_expr->get_value().convert_to<std::size_t>();

                auto type = ctx.get_sized_integer_type(size);
                expr->replace_with(make_expression_ref(
                    type->get_expression(), expr->get_scope(), expr->get_name(), expr->get_ast_info()));

                return make_ready_future();
            });

        auto sized_int_expr = make_function_expression(sized_int.get(), nullptr, lex_scope, U"sized_int");
        keepalive_list.emplace_back(std::move(sized_int));

        add_symbol(U"sized_int", sized_int_expr.get());
        keepalive_list.emplace_back(std::move(sized_int_expr));

        auto instance_type_expr = make_runtime_value(builtin_types().type, nullptr, std::nullopt);
        auto default_ = make_function("default");
        default_->set_return_type(instance_type_expr.get());
        default_->set_parameters(unique_expr_list(std::move(instance_type_expr)));

        default_->add_analysis_hook(
            [](analysis_context & ctx, call_expression * expr, std::vector<expression *> args) {
                assert(args.size() == 2 && args[1]->is_constant());
                auto instance_type_expr = args[1]->as<type_expression>();
                assert(instance_type_expr);
                auto instance_type = dynamic_cast<typeclass_instance_type *>(instance_type_expr->get_value());
                assert(instance_type);

                return ctx.default_instances_future()
                    .then([&ctx, call = expr, inst = instance_type] {
                        return ctx.resolve_default_instance(
                            call->get_range(), call->get_scope(), call->get_name(), inst);
                    })
                    .then([&ctx, call = expr, instance_type](auto && inst_expr) {
                        assert(inst_expr->get_type() == instance_type);
                        call->replace_with(std::move(inst_expr));
                    });
            });

        auto default_expr = make_function_expression(default_.get(), nullptr, lex_scope, U"default");
        keepalive_list.emplace_back(std::move(default_));

        add_symbol(U"default", default_expr.get());
        keepalive_list.emplace_back(std::move(default_expr));
    }
}
}
