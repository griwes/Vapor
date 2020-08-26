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

#include "vapor/analyzer/expressions/overload_set.h"

#include <numeric>

#include <boost/algorithm/string/join.hpp>

#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/codegen/ir/function.h"
#include "vapor/codegen/ir/type.h"
#include "vapor/parser.h"

#include "expressions/overload_set.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unordered_set<expression *> overload_set_expression_base::get_associated_entities() const
    {
        std::unordered_set<expression *> ret;

        // FIXME: this should also include all types that appear in signatures of the functions
        // TODO: fix the above and add tests that verify it
        // further FIXME: and also all types that appear in the definitions of functions with exported bodies
        ret.insert(get_overload_set()->get_type()->get_expression());

        return ret;
    }

    overload_set_expression::overload_set_expression(scope * lex_scope, std::u32string name)
        : overload_set_expression_base{ lex_scope, name },
          _oset{ std::make_unique<overload_set>(lex_scope->clone_for_type(U"oset$" + name)) }
    {
        _set_type(_oset->get_type());
    }

    overload_set_expression::overload_set_expression(std::shared_ptr<overload_set> t,
        scope * lex_scope,
        std::u32string name)
        : overload_set_expression_base{ lex_scope, std::move(name) }, _oset{ std::move(t) }
    {
        _set_type(_oset->get_type());
    }

    bool overload_set_expression::is_constant() const
    {
        auto overloads = _oset->get_overloads();
        return std::all_of(
            overloads.begin(), overloads.end(), [](auto && fn) { return !fn->vtable_slot().has_value(); });
    }

    std::unique_ptr<expression> overload_set_expression::_clone_expr(replacements & repl) const
    {
        return std::make_unique<overload_set_expression>(_oset, get_scope(), get_name().value());
    }

    statement_ir overload_set_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        // assert(0); // this function looks _really_ wrong
        assert(get_type()->get_scope());
        auto var = codegen::ir::make_variable(get_type()->codegen_type(ctx));
        if (get_name())
        {
            var->name = _oset->get_name();
        }
        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() },
            {},
            codegen::ir::value{ std::move(var) } } };
    }

    constant_init_ir overload_set_expression::_constinit_ir(ir_generation_context & ctx) const
    {
        assert(is_constant());

        auto type = get_type()->codegen_type(ctx);
        // TODO: figure out how to get rid of this dynamic pointer cast that is really irritating here
        return codegen::ir::struct_value{ std::dynamic_pointer_cast<codegen::ir::user_type>(type), {} };
    }

    declaration_ir overload_set_expression::declaration_codegen_ir(ir_generation_context & ctx) const
    {
        return { { std::get<std::shared_ptr<codegen::ir::variable>>(codegen_ir(ctx).back().result) } };
    }

    std::unique_ptr<google::protobuf::Message> overload_set_expression::_generate_interface() const
    {
        return std::make_unique<proto::overload_set>();
    }

    refined_overload_set_expression::refined_overload_set_expression(
        std::shared_ptr<refined_overload_set> oset,
        scope * lex_scope,
        std::u32string name)
        : overload_set_expression_base{ lex_scope, std::move(name) }, _oset{ std::move(oset) }
    {
        _set_type(_oset->get_type());
    }

    refined_overload_set_expression::refined_overload_set_expression(overload_set * base,
        scope * lex_scope,
        std::u32string name)
        : overload_set_expression_base{ lex_scope, std::move(name) },
          _oset{ std::make_unique<refined_overload_set>(lex_scope, base) }
    {
        _set_type(_oset->get_type());
    }

    bool refined_overload_set_expression::is_constant() const
    {
        return true;
    }

    function * refined_overload_set_expression::get_vtable_entry(std::size_t id) const
    {
        return _oset->get_vtable_entry(id);
    }

    std::unique_ptr<expression> refined_overload_set_expression::_clone_expr(replacements &) const
    {
        return std::make_unique<refined_overload_set_expression>(_oset, get_scope(), get_name().value());
    }

    statement_ir refined_overload_set_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() },
            {},
            _constinit_ir(ctx) } };
    }

    constant_init_ir refined_overload_set_expression::_constinit_ir(ir_generation_context & ctx) const
    {
        auto type = get_type()->codegen_type(ctx);
        // TODO: figure out how to get rid of this dynamic pointer cast that is really irritating here
        auto val = codegen::ir::struct_value{ std::dynamic_pointer_cast<codegen::ir::user_type>(type), {} };
        assert(val.type);

        auto overloads = get_overloads();
        val.fields.reserve(overloads.size());

        for (auto && fn : overloads)
        {
            val.fields.push_back(fn->pointer_ir(ctx));
            if (_is_exported)
            {
                fn->mark_exported();
            }
            ctx.add_function_to_generate(fn);
        }

        return val;
    }

    std::unique_ptr<google::protobuf::Message> refined_overload_set_expression::_generate_interface() const
    {
        assert(0);
    }

    namespace _detail
    {
        template<typename F>
        auto create_overload_set(scope * lex_scope, std::u32string name, F create)
        {
            auto type_name = U"oset$" + name;

            auto oset = create();
            lex_scope->init(name, make_symbol(name, oset.get()));

            auto type = oset->get_type();
            lex_scope->init(type_name, make_symbol(type_name, type->get_expression()));

            return oset;
        }
    }

    std::unique_ptr<overload_set_expression> create_overload_set(scope * lex_scope, std::u32string name)
    {
        return _detail::create_overload_set(
            lex_scope, name, [&] { return std::make_unique<overload_set_expression>(lex_scope, name); });
    }

    std::unique_ptr<refined_overload_set_expression> create_refined_overload_set(scope * lex_scope,
        std::u32string name,
        overload_set * base)
    {
        return _detail::create_overload_set(lex_scope, name, [&] {
            return std::make_unique<refined_overload_set_expression>(base, lex_scope, name);
        });
    }

    namespace _detail
    {
        template<typename F, typename G>
        auto get_overload_set(scope * lex_scope, std::u32string name, F create, G clone)
        {
            auto symbol = lex_scope->try_get(name);

            if (!symbol)
            {
                return create();
            }

            return clone(symbol.value()->get_expression());
        }

        std::unique_ptr<overload_set_expression> clone_oset_expr(expression * expr)
        {
            using T = overload_set_expression;
            auto downcasted = expr->as<T>();
            assert(downcasted);
            return std::make_unique<T>(*downcasted);
        }

        std::unique_ptr<refined_overload_set_expression> clone_refined_oset_expr(expression * expr,
            overload_set * base)
        {
            using T = refined_overload_set_expression;
            auto downcasted = expr->as<T>();
            assert(downcasted);
            assert(!base || downcasted->get_overload_set()->get_base() == base);
            return std::make_unique<T>(*downcasted);
        }
    }

    std::unique_ptr<overload_set_expression_base> get_overload_set(scope * lex_scope, std::u32string name)
    {
        return _detail::get_overload_set(
            lex_scope,
            name,
            [&]() -> std::unique_ptr<overload_set_expression_base> {
                return create_overload_set(lex_scope, name);
            },
            [](expression * expr) -> std::unique_ptr<overload_set_expression_base> {
                // this will dynamic_cast twice, but until this proves problematic, it seems fine
                if (expr->as<overload_set_expression>())
                {
                    return _detail::clone_oset_expr(expr);
                }
                return _detail::clone_refined_oset_expr(expr, nullptr);
            });
    }

    std::unique_ptr<overload_set_expression> get_overload_set_special(scope * lex_scope, std::u32string name)
    {
        return _detail::get_overload_set(
            lex_scope, name, [&] { return create_overload_set(lex_scope, name); }, _detail::clone_oset_expr);
    }

    std::unique_ptr<refined_overload_set_expression> get_refined_overload_set(scope * lex_scope,
        std::u32string name,
        overload_set * base)
    {
        return _detail::get_overload_set(
            lex_scope,
            std::move(name),
            [&] { return create_refined_overload_set(lex_scope, name, base); },
            [&](expression * expr) { return _detail::clone_refined_oset_expr(expr, base); });
    }

    unresolved_overload_set_expression::unresolved_overload_set_expression(precontext & ctx,
        const proto::user_defined_reference * udr,
        scope * lex_scope)
        : overload_set_expression_base{ lex_scope,
              utf32(udr->name()).substr(lex_scope->get_scoped_name().size()) },
          _ctx{ &ctx },
          _udr{ udr->name() }
    {
        logger::dlog() << "uhhh: " << utf8(get_name().value()) << ", " << utf8(lex_scope->get_scoped_name());
    }

    future<> unresolved_overload_set_expression::_analyze(analysis_context &)
    {
        auto && oset = _ctx->imported_overload_sets[_udr];
        assert(oset);
        _resolved = std::make_unique<overload_set_expression>(oset, get_scope(), get_name().value());
        _set_type(oset->get_type());

        return make_ready_future();
    }

    overload_set_base * unresolved_overload_set_expression::get_overload_set() const
    {
        assert(_resolved);
        return _resolved->get_overload_set();
    }

    bool unresolved_overload_set_expression::is_constant() const
    {
        assert(_resolved);
        return _resolved->is_constant();
    }

    function * unresolved_overload_set_expression::get_vtable_entry(std::size_t id) const
    {
        assert(_resolved);
        return _resolved->get_vtable_entry(id);
    }

    std::unique_ptr<expression> unresolved_overload_set_expression::_clone_expr(replacements & repl) const
    {
        assert(_resolved);
        return repl.claim(_resolved.get());
    }

    statement_ir unresolved_overload_set_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        assert(_resolved);
        return _resolved->codegen_ir(ctx);
    }

    constant_init_ir unresolved_overload_set_expression::_constinit_ir(ir_generation_context & ctx) const
    {
        assert(_resolved);
        return _resolved->constinit_ir(ctx);
    }

    std::unique_ptr<unresolved_overload_set_expression> make_unresolved_overload_set_expression(
        precontext & ctx,
        const proto::user_defined_reference * udr,
        scope * lex_scope)
    {
        return std::make_unique<unresolved_overload_set_expression>(ctx, udr, lex_scope);
    }
}
}
