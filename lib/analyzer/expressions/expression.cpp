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

#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/binary.h"
#include "vapor/analyzer/expressions/boolean.h"
#include "vapor/analyzer/expressions/closure.h"
#include "vapor/analyzer/expressions/import.h"
#include "vapor/analyzer/expressions/integer.h"
#include "vapor/analyzer/expressions/member_access.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/expressions/postfix.h"
#include "vapor/analyzer/expressions/struct_literal.h"
#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/expressions/typeclass_instance.h"
#include "vapor/analyzer/expressions/unary.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/parser/expr.h"

#include "entity.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<expression> preanalyze_expression(precontext & ctx,
        const parser::_v1::expression & expr,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name)
    {
        return std::get<0>(fmap(expr.expression_value,
            make_overload_set(
                [](const parser::string_literal & string) -> std::unique_ptr<expression> {
                    assert(0);
                    return nullptr;
                },

                [&](const parser::integer_literal & integer) -> std::unique_ptr<expression> {
                    return make_integer_constant(integer, lex_scope, std::move(canonical_name));
                },

                [&](const parser::boolean_literal & boolean) -> std::unique_ptr<expression> {
                    return make_boolean_constant(boolean, lex_scope, std::move(canonical_name));
                },

                [&](const parser::postfix_expression & postfix) -> std::unique_ptr<expression> {
                    auto pexpr =
                        preanalyze_postfix_expression(ctx, postfix, lex_scope, std::move(canonical_name));
                    return pexpr;
                },

                [&](const parser::import_expression & import) -> std::unique_ptr<expression> {
                    auto impexpr = preanalyze_import(ctx, import, lex_scope, std::move(canonical_name));
                    return impexpr;
                },

                [&](const parser::lambda_expression & lambda_expr) -> std::unique_ptr<expression> {
                    auto lambda = preanalyze_closure(ctx, lambda_expr, lex_scope, std::move(canonical_name));
                    return lambda;
                },

                [](const parser::unary_expression & unary_expr) -> std::unique_ptr<expression> {
                    assert(0);
                    return std::unique_ptr<unary_expression>();
                },

                [&](const parser::binary_expression & binary_expr) -> std::unique_ptr<expression> {
                    auto binexpr =
                        preanalyze_binary_expression(ctx, binary_expr, lex_scope, std::move(canonical_name));
                    return binexpr;
                },

                [&](const parser::struct_literal & struct_lit) -> std::unique_ptr<expression> {
                    auto struct_expr =
                        preanalyze_struct_literal(ctx, struct_lit, lex_scope, std::move(canonical_name));
                    return struct_expr;
                },

                [&](const parser::member_expression & mexpr) -> std::unique_ptr<expression> {
                    auto memexpr =
                        preanalyze_member_access_expression(ctx, mexpr, lex_scope, std::move(canonical_name));
                    return memexpr;
                },

                [&](const parser::typeclass_literal & tc) -> std::unique_ptr<expression> {
                    auto tclit = preanalyze_typeclass_literal(ctx, tc, lex_scope, std::move(canonical_name));
                    return tclit;
                },

                [&](const parser::instance_literal & inst) -> std::unique_ptr<expression> {
                    auto instlit =
                        preanalyze_instance_literal(ctx, inst, lex_scope, false, std::move(canonical_name));
                    return instlit;
                },

                [](auto &&) -> std::unique_ptr<expression> { assert(0); })));
    }

    future<expression *> expression::simplify_expr(recursive_context ctx)
    {
        return ctx.proper.get_future_or_init(this, [&]() {
            return make_ready_future()
                .then([this, ctx]() { return _simplify_expr(ctx); })
                .then([this](auto && expr) {
                    logger::dlog(logger::trace)
                        << "Simplified " << this << " (" << typeid(*this).name() << ") to " << expr << " ("
                        << (expr ? typeid(*expr).name() : "?") << ")";
                    return expr;
                });
        });
    }

    bool expression::is_constant() const
    {
        auto repl = _get_replacement();
        assert(repl);
        return repl != this && repl->is_constant();
    }

    bool expression::is_equal(const expression * rhs) const
    {
        if (this == rhs && _is_pure())
        {
            return true;
        }

        if (_is_equal(rhs) || rhs->_is_equal(this))
        {
            return true;
        }

        auto this_replacement = _get_replacement();
        auto rhs_replacement = rhs->_get_replacement();
        if (this != this_replacement || rhs != rhs_replacement)
        {
            return this_replacement->_is_equal(rhs_replacement)
                || rhs_replacement->_is_equal(this_replacement);
        }

        return false;
    }

    bool expression::is_different_constant(const expression * rhs)
    {
        bool is_c = is_constant();

        if (is_c ^ rhs->is_constant())
        {
            return true;
        }

        return is_c && !is_equal(rhs);
    }

    std::unique_ptr<expression> expression::convert_to(type * target) const
    {
        if (get_type() == target)
        {
            assert(0); // I don't know whether I want to support this
        }

        auto repl = _get_replacement();
        if (repl == this)
        {
            return nullptr;
        }

        return repl->convert_to(target);
    }

    bool expression::is_member() const
    {
        return false;
    }

    bool expression::is_member_assignment() const
    {
        auto repl = _get_replacement();
        return repl != this && repl->is_member_assignment();
    }

    bool expression::is_member_access() const
    {
        return false;
    }

    expression * expression::get_member(const std::u32string & name) const
    {
        auto repl = _get_replacement();
        if (repl == this)
        {
            return nullptr;
        }

        return repl->get_member(name);
    }

    function * expression::get_vtable_entry(std::size_t) const
    {
        return nullptr;
    }

    std::optional<std::u32string_view> expression::get_entity_name() const
    {
        if (!_entity_name)
        {
            _entity_name = _get_entity_name();
        }

        if (_entity_name)
        {
            return _entity_name;
        }

        return std::nullopt;
    }

    void expression::generate_interface(proto::entity & entity) const
    {
        entity.set_allocated_type(_type->generate_interface_reference().release());

        auto message = _generate_interface();

        auto dynamic_switch = [&](auto &&... pairs) {
            ((dynamic_cast<typename decltype(pairs.first)::type *>(message.get())
                 && pairs.second(dynamic_cast<typename decltype(pairs.first)::type *>(message.release())))
                    || ... || [&]() -> bool {
                auto m = message.get();
                throw exception{ logger::crash } << "unhandled serialized expression type: `"
                                                 << typeid(*m).name() << "`";
            }());
        };

#define HANDLE_TYPE(type, field_name)                                                                        \
    std::make_pair(id<proto::type>(), [&](auto ptr) {                                                        \
        entity.set_allocated_##field_name(ptr);                                                              \
        return true;                                                                                         \
    })

        dynamic_switch(HANDLE_TYPE(type, type_value),
            HANDLE_TYPE(overload_set, overload_set),
            HANDLE_TYPE(typeclass, typeclass),
            HANDLE_TYPE(typeclass_instance, typeclass_instance));

#undef HANDLE_TYPE
    }

    future<expression *> simplification_loop(analysis_context & ctx, std::unique_ptr<expression> & uptr)
    {
        auto cont = [&uptr, ctx = std::make_shared<simplification_context>(*ctx.results)](
                        auto self) -> future<expression *> {
            return uptr->simplify_expr({ *ctx }).then(
                [&uptr, ctx, self](auto && simpl) -> future<expression *> {
                    replace_uptr(uptr, simpl, *ctx);

                    if (uptr->is_constant() || !ctx->did_something_happen())
                    {
                        return make_ready_future<expression *>(uptr.get());
                    }

                    auto & res = ctx->results;
                    ctx->~simplification_context();
                    new (&*ctx) simplification_context(res);
                    return self(self);
                });
        };

        return cont(cont);
    }

    std::optional<std::u32string> expression::_get_entity_name() const
    {
        if (!_name)
        {
            return std::nullopt;
        }

        return std::u32string{ _lex_scope->get_scoped_name() } + _name.value();
    }

    future<> expression::_analyze(analysis_context &)
    {
        assert(_type);
        return make_ready_future();
    }

    std::unique_ptr<statement> expression::_clone(replacements & repl) const
    {
        return _clone_expr(repl);
    }

    future<statement *> expression::_simplify(recursive_context ctx)
    {
        return simplify_expr(ctx).then([&](auto && simplified) -> statement * { return simplified; });
    }

    future<expression *> expression::_simplify_expr(recursive_context)
    {
        return make_ready_future(this);
    }

    bool expression::_is_equal([[maybe_unused]] const expression * expr) const
    {
        return false;
    }

    bool expression::_is_pure() const
    {
        return true;
    }

    std::unique_ptr<google::protobuf::Message> expression::_generate_interface() const
    {
        auto replacement = _get_replacement();

        if (this == replacement)
        {
            assert(0);
        }

        return replacement->_generate_interface();
    }
}
}
