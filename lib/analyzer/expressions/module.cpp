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

#include "vapor/analyzer/expressions/module.h"

#include <reaver/future_get.h>
#include <reaver/prelude/monad.h>
#include <reaver/traits.h>

#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/expressions/typeclass_instance.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/types/sized_integer.h"
#include "vapor/parser.h"
#include "vapor/parser/module.h"

#include "entity.pb.h"
#include "module.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<module> preanalyze_module(precontext & ctx,
        const parser::module & parse,
        scope * lex_scope)
    {
        std::string cumulative_name;
        module_type * type = nullptr;

        // TODO: integrate this with the same stuff in import_from_ast
        auto name = fmap(parse.name.id_expression_value, [&](auto && elem) {
            auto name_part = elem.value.string;

            cumulative_name = cumulative_name + (cumulative_name.empty() ? "" : ".") + utf8(name_part);
            auto & saved = ctx.modules[cumulative_name];

            if (!saved)
            {
                auto scope = lex_scope->clone_for_module(name_part);
                auto old_scope = std::exchange(lex_scope, scope.get());

                auto type_uptr = std::make_unique<module_type>(std::move(scope), utf8(name_part));
                type = type_uptr.get();

                saved = make_entity(std::move(type_uptr), old_scope, name_part);
                saved->mark_local();
                assert(old_scope->init(name_part, make_symbol(name_part, saved.get())));
            }

            else
            {
                type = dynamic_cast<module_type *>(saved->get_type());
                assert(type);
                lex_scope = type->get_scope();
            }

            return name_part;
        });

        auto statements = fmap(parse.statements, [&](const auto & statement) {
            auto scope_ptr = lex_scope;
            auto ret = preanalyze_statement(ctx, statement, scope_ptr);
            assert(lex_scope == scope_ptr);
            return ret;
        });

        return std::make_unique<module>(
            make_node(parse), lex_scope, name.back(), std::move(name), type, std::move(statements));
    }

    module::module(ast_node parse,
        scope * lex_scope,
        std::u32string canonical_name,
        std::vector<std::u32string> name,
        module_type * type,
        std::vector<std::unique_ptr<statement>> stmts)
        : expression{ type, lex_scope, std::move(canonical_name) },
          _parse{ parse },
          _type{ type },
          _name{ std::move(name) },
          _statements{ std::move(stmts) }
    {
    }

    future<> module::_analyze(analysis_context & ctx)
    {
        return when_all(fmap(_statements, [&](auto && stmt) {
            return stmt->analyze(ctx);
        })).then([this, &ctx] {
            if (name() == U"main")
            {
                // set entry(int32) as the entry point
                if (auto entry = _type->get_scope()->try_get(U"entry"))
                {
                    auto type = entry.value()->get_type();
                    return type->get_candidates(lexer::token_type::round_bracket_open)
                        .then([&ctx, expr = entry.value()->get_expression()](auto && overloads) {
                            // maybe this can be relaxed in the future?
                            assert(overloads.size() == 1);
                            assert(overloads[0]->parameters().size() == 1);
                            assert(
                                overloads[0]->parameters()[0]->get_type() == ctx.get_sized_integer_type(32));

                            overloads[0]->mark_as_entry(ctx, expr);
                        });
                }
            }

            return make_ready_future();
        });
    }

    future<expression *> module::_simplify_expr(recursive_context ctx)
    {
        return when_all(fmap(_statements, [&](auto && stmt) {
            return stmt->simplify(ctx).then(
                [&ctx = ctx.proper, &stmt](auto && simplified) { replace_uptr(stmt, simplified, ctx); });
        })).then([this]() -> expression * { return this; });
    }

    void module::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "module";
        print_address_range(os, this);
        os << '\n';

        if (_statements.size())
        {
            auto stmts_ctx = ctx.make_branch(true);
            os << styles::def << stmts_ctx << styles::subrule_name << "statements\n";

            std::size_t idx = 0;
            for (auto && stmt : _statements)
            {
                stmt->print(os, stmts_ctx.make_branch(++idx == _statements.size()));
            }
        }
    }

    void module::generate_interface(proto::module & mod) const
    {
        auto own_scope = _type->get_scope();

        auto name = fmap(_name, [](auto && arg) { return utf8(std::forward<decltype(arg)>(arg)); });
        std::copy(name.begin(), name.end(), RepeatedFieldBackInserter(mod.mutable_name()));

        std::unordered_set<expression *> associated_entities;
        std::unordered_set<expression *> exported_entities;
        std::unordered_set<expression *> named_exports;

        auto & mut_symbols = *mod.mutable_symbols();

        for (auto && symbol : own_scope->declared_symbols())
        {
            if (!symbol->is_exported())
            {
                continue;
            }

            auto expr = symbol->get_expression();
            exported_entities.insert(expr);
            named_exports.insert(expr);

            auto associated = expr->get_associated_entities();
            associated_entities.insert(associated.begin(), associated.end());
        }

        for (auto && associated : associated_entities)
        {
            if (auto scope = [&]() -> class scope * {
                    if (auto && type_expr = associated->as<type_expression>())
                    {
                        return type_expr->get_value()->get_scope();
                    }

                    if (auto && tc_expr = associated->as<typeclass_expression>())
                    {
                        return tc_expr->get_typeclass()->get_scope();
                    }

                    return nullptr;
                }())
            {
                if ([&] {
                        while (scope)
                        {
                            if (scope == own_scope)
                            {
                                return true;
                            }
                            scope = scope->parent();
                        }
                        return false;
                    }())
                {
                    exported_entities.insert(associated);
                }
            }

            else
            {
                assert(0);
            }
        }

        for (auto && entity : exported_entities)
        {
            auto ent_name = entity->get_entity_name();

            if (ent_name)
            {
                auto & symb = mut_symbols[utf8(ent_name.value())];
                entity->generate_interface(symb);

                auto it = named_exports.find(entity);
                if (it != named_exports.end())
                {
                    symb.set_is_name_exported(true);
                }
            }
            else
            {
                auto instance = entity->as<typeclass_instance_expression>();
                assert(instance);
                auto exported = mod.add_default_instances();
                instance->generate_interface(*exported);
            }
        }
    }
}
}
