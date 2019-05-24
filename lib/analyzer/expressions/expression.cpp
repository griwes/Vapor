/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2019 Michał "Griwes" Dominiak
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
        scope * lex_scope)
    {
        return std::get<0>(fmap(expr.expression_value,
            make_overload_set(
                [](const parser::string_literal & string) -> std::unique_ptr<expression> {
                    assert(0);
                    return nullptr;
                },

                [](const parser::integer_literal & integer) -> std::unique_ptr<expression> {
                    return make_integer_constant(integer);
                },

                [](const parser::boolean_literal & boolean) -> std::unique_ptr<expression> {
                    return make_boolean_constant(boolean);
                },

                [&](const parser::postfix_expression & postfix) -> std::unique_ptr<expression> {
                    auto pexpr = preanalyze_postfix_expression(ctx, postfix, lex_scope);
                    return pexpr;
                },

                [&](const parser::import_expression & import) -> std::unique_ptr<expression> {
                    auto impexpr = preanalyze_import(ctx, import, lex_scope, import_mode::expression);
                    return impexpr;
                },

                [&](const parser::lambda_expression & lambda_expr) -> std::unique_ptr<expression> {
                    auto lambda = preanalyze_closure(ctx, lambda_expr, lex_scope);
                    return lambda;
                },

                [](const parser::unary_expression & unary_expr) -> std::unique_ptr<expression> {
                    assert(0);
                    return std::unique_ptr<unary_expression>();
                },

                [&](const parser::binary_expression & binary_expr) -> std::unique_ptr<expression> {
                    auto binexpr = preanalyze_binary_expression(ctx, binary_expr, lex_scope);
                    return binexpr;
                },

                [&](const parser::struct_literal & struct_lit) -> std::unique_ptr<expression> {
                    auto struct_expr = preanalyze_struct_literal(ctx, struct_lit, lex_scope);
                    return struct_expr;
                },

                [&](const parser::member_expression & mexpr) -> std::unique_ptr<expression> {
                    auto memexpr = preanalyze_member_access_expression(ctx, mexpr, lex_scope);
                    return memexpr;
                },

                [&](const parser::typeclass_literal & tc) -> std::unique_ptr<expression> {
                    auto tclit = preanalyze_typeclass_literal(ctx, tc, lex_scope);
                    return tclit;
                },

                [&](const parser::instance_literal & inst) -> std::unique_ptr<expression> {
                    auto instlit = preanalyze_instance_literal(ctx, inst, lex_scope);
                    return instlit;
                },

                [](auto &&) -> std::unique_ptr<expression> { assert(0); })));
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
}
}
