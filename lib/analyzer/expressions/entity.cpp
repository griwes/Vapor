/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/entity.h"
#include "vapor/analyzer/expressions/overload_set.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/expressions/typeclass_instance.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/overload_set.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/semantic/typeclass.h"
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/module.h"
#include "vapor/analyzer/types/unresolved.h"

#include "entity.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    entity::entity(type * t, scope * lex_scope, std::u32string name, std::unique_ptr<expression> wrapped)
        : expression{ t, lex_scope, std::move(name) }, _wrapped{ std::move(wrapped) }
    {
    }

    entity::entity(std::unique_ptr<type> t,
        scope * lex_scope,
        std::u32string name,
        std::unique_ptr<expression> wrapped)
        : expression{ t.get(), lex_scope, std::move(name) },
          _owned{ std::move(t) },
          _wrapped{ std::move(wrapped) }
    {
    }

    entity::entity(std::shared_ptr<unresolved_type> res,
        scope * lex_scope,
        std::u32string name,
        std::unique_ptr<expression> wrapped)
        : expression{ lex_scope, std::move(name) },
          _unresolved{ std::move(res) },
          _wrapped{ std::move(wrapped) }
    {
        if (auto t = _unresolved.value()->get_resolved())
        {
            _set_type(t);
            _unresolved.reset();
        }
    }

    void entity::print(std::ostream & os, print_context ctx) const
    {
        assert(0);
    }

    future<> entity::_analyze(analysis_context & ctx)
    {
        auto fut = [&] {
            if (_unresolved)
            {
                return _unresolved.value()->resolve(ctx).then([&] {
                    _set_type(_unresolved.value()->get_resolved());
                    _unresolved.reset();
                });
            }

            return make_ready_future();
        }();

        if (_wrapped)
        {
            fut = fut.then([&, this] { return _wrapped->analyze(ctx); });
        }

        return fut;
    }

    future<expression *> entity::_simplify_expr(recursive_context ctx)
    {
        return make_ready_future<expression *>(this);
    }

    std::unique_ptr<expression> entity::_clone_expr(replacements & repl) const
    {
        assert(0);
    }

    statement_ir entity::_codegen_ir(ir_generation_context & ctx) const
    {
        if (_wrapped)
        {
            return _wrapped->codegen_ir(ctx);
        }

        return { codegen::ir::instruction{ std::nullopt,
            std::nullopt,
            { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() },
            {},
            codegen::ir::make_variable(get_type()->codegen_type(ctx)) } };
    }

    constant_init_ir entity::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }

    declaration_ir entity::declaration_codegen_ir(ir_generation_context & ctx) const
    {
        if (get_type()->is_meta())
        {
            return {};
        }

        if (_wrapped)
        {
            // trigger side-effects of generating the IR for the underlying entity
            // such as emitting declarations of typeclass instance functions
            static_cast<void>(_wrapped->codegen_ir(ctx));
            if (_wrapped->is_constant())
            {
                static_cast<void>(_wrapped->constinit_ir(ctx));
            }
        }
        return { codegen::ir::make_variable(get_type()->codegen_type(ctx),
            fmap(get_entity_name(), [](auto && sv) { return std::u32string{ sv }; })) };
    }

    std::vector<codegen::ir::entity> entity::module_codegen_ir(ir_generation_context & ctx) const
    {
        assert(_owned);
        assert(dynamic_cast<module_type *>(_owned.value().get()));

        std::u32string own_scope{ _owned.value()->get_scope()->get_scoped_name() };

        auto mod = mbind(_owned.value()->get_scope()->symbols_in_order(), [&](auto && symbol) {
            return fmap(symbol->codegen_ir(ctx), [&](auto && decl) {
                return fmap(decl,
                    make_overload_set(
                        [&](std::shared_ptr<codegen::ir::variable> symb) {
                            symb->imported = !_is_local;
                            symb->declared = !_is_local;
                            return symb;
                        },
                        [&](auto && symb) { return symb; }));
            });
        });

        return mod;
    }

    std::unique_ptr<entity> get_entity(precontext & ctx, const proto::entity & ent)
    {
        auto type = get_imported_type_ref(ctx, ent.type());

        std::unique_ptr<expression> expr;

        switch (ent.value_case())
        {
            case proto::entity::kTypeValue:
                expr = get_imported_type(ctx, ent.type_value());
                break;

            case proto::entity::kOverloadSet:
                expr = make_unresolved_overload_set_expression(
                    ctx, &ent.type().user_defined(), ctx.current_lex_scope);
                break;

            case proto::entity::kTypeclass:
            {
                auto tc = import_typeclass(ctx, ent.typeclass());
                expr = std::make_unique<typeclass_expression>(
                    imported_ast_node(ctx, ent.range()), std::move(tc));
                break;
            }

            case proto::entity::kTypeclassInstance:
            {
                auto inst = import_typeclass_instance(ctx, type, ent.typeclass_instance(), false);
                expr = std::make_unique<typeclass_instance_expression>(
                    imported_ast_node(ctx, ent.range()), std::move(inst));
                break;
            }

            default:
                throw exception{ logger::fatal } << "unknown expression kind of symbol `"
                                                 << ctx.module_stack.back() << "." << ctx.current_symbol
                                                 << "` in imported file "
                                                 << ctx.module_path_stack.back().module_file_path;
        }

        return std::get<0>(fmap(type, [&](auto && type) {
            return std::make_unique<entity>(
                std::move(type), ctx.current_lex_scope, utf32(ctx.current_symbol), std::move(expr));
        }));
    }
}
}
