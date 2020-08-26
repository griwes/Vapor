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

#include "vapor/analyzer/semantic/typeclass_instance.h"

#include <reaver/future_get.h>

#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/typeclass.h"

#include "expressions/typeclass.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_instance> make_typeclass_instance(precontext & ctx,
        const parser::instance_literal & parse,
        scope * lex_scope,
        bool is_default,
        std::optional<std::u32string> canonical_name)
    {
        auto scope = [&] {
            if (is_default)
            {
                return lex_scope->_clone_for_default_instance();
            }

            assert(canonical_name);
            auto name = std::move(canonical_name.value());

            return lex_scope->clone_for_type(std::move(name));
        }();
        auto scope_ptr = scope.get();

        auto ret = std::make_unique<typeclass_instance>(make_node(parse),
            std::move(scope),
            fmap(parse.typeclass_name.id_expression_value, [&](auto && t) { return t.value.string; }),
            fmap(parse.arguments, [&](auto && arg) { return preanalyze_expression(ctx, arg, lex_scope); }));
        if (is_default)
        {
            ret->mark_default();
        }
        return ret;
    }

    std::unique_ptr<typeclass_instance> import_typeclass_instance(precontext & ctx,
        imported_type type,
        const proto::typeclass_instance & inst,
        bool is_default)
    {
        auto ret = std::make_unique<typeclass_instance>(imported_ast_node(ctx, inst.range()),
            is_default ? ctx.current_lex_scope->_clone_for_default_instance()
                       : ctx.current_lex_scope->clone_for_type(
                           utf32(ctx.current_symbol).substr(ctx.current_lex_scope->get_scoped_name().size())),
            std::move(type));
        if (is_default)
        {
            ret->mark_default();
        }
        return ret;
    }

    typeclass_instance::typeclass_instance(ast_node parse,
        std::unique_ptr<scope> member_scope,
        std::vector<std::u32string> typeclass_name,
        std::vector<std::unique_ptr<expression>> arguments)
        : _node{ parse },
          _scope{ std::move(member_scope) },
          _typeclass_reference{ std::move(typeclass_name) },
          _arguments{ std::move(arguments) }
    {
    }

    typeclass_instance::typeclass_instance(ast_node parse,
        std::unique_ptr<scope> member_scope,
        imported_type type)
        : _node{ parse },
          _scope{ std::move(member_scope) },
          _typeclass_reference{ std::move(type) },
          _imported{ true }
    {
    }

    std::vector<expression *> typeclass_instance::get_arguments() const
    {
        return fmap(_arguments, [](auto && arg) { return arg.get(); });
    }

    std::vector<type *> typeclass_instance::get_argument_values() const
    {
        return fmap(_arguments, [](auto && arg) {
            auto type_expr = arg->template as<type_expression>();
            assert(type_expr);
            return type_expr->get_value();
        });
    }

    future<> typeclass_instance::simplify_arguments(analysis_context & ctx)
    {
        return when_all(fmap(_arguments, [&](auto && arg) {
            return simplification_loop(ctx, arg);
        })).then([](auto &&) {});
    }

    std::vector<function_definition *> typeclass_instance::get_member_function_defs() const
    {
        return fmap(_member_function_definitions, [](auto && def) { return def.get(); });
    }

    void typeclass_instance::set_type(typeclass_instance_type * type)
    {
        if (_arguments.empty())
        {
            // fix arguments for an imported typeclass instance
            _arguments = fmap(type->get_arguments(), [&](auto && type) {
                return make_expression_ref(type->get_expression(), nullptr, std::nullopt, std::nullopt);
            });
        }

        for (auto && [oset_name, oset] : type->get_overload_sets())
        {
            _member_overload_set_exprs.push_back(create_refined_overload_set(_scope.get(), oset_name, oset));
        }

        // close here, because if the delayed preanalysis later *adds* new members, then we have a bug... the
        // assertion that checks for closeness of the scope needs to be somehow weakened here, to allow for
        // more sensible error reporting than `assert`
        _scope->close();

        if (_default)
        {
            _scope->set_name(U"defaults$" + type->codegen_name());
        }

        _type = type;
    }

    function_definition_handler typeclass_instance::get_function_definition_handler()
    {
        return [&](precontext & ctx, const parser::function_definition & parse) {
            auto scope = get_scope();
            auto func = preanalyze_function_definition(ctx, parse, scope, _type);
            assert(scope == get_scope());
            _member_function_definitions.push_back(std::move(func));
        };
    }

    void typeclass_instance::import_default_definitions(analysis_context & ctx, bool is_imported)
    {
        replacements repl;
        std::unordered_map<function *, block *> function_block_defs;

        for (auto && roset_expr : _member_overload_set_exprs)
        {
            auto && roset = roset_expr->get_overload_set();
            roset->resolve_overrides();

            auto scope_gen = [this, name = roset->get_type()->get_scope()->get_name()]() {
                if (!_default)
                {
                    return std::u32string{ this->get_scope()->get_entity_name() } + U"."
                        + std::u32string{ name };
                }

                return std::u32string{ U"defaults$" } + std::u32string{ get_type()->codegen_name() } + U"."
                    + std::u32string{ name };
            };

            auto && base = roset->get_base();
            for (auto && fn : base->get_overloads())
            {
                assert(fn->vtable_slot());
                if (auto refinement = roset->get_vtable_entry(fn->vtable_slot().value()))
                {
                    repl.add_replacement(fn, refinement);
                    continue;
                }

                assert(is_imported || fn->get_body());

                _function_specialization fn_spec;
                fn_spec.spec = make_function(fn->get_explanation(), fn->get_range());

                fn_spec.spec->set_name(scope_gen() + U".call");

                repl.add_replacement(fn, fn_spec.spec.get());
                fn_spec.spec->set_return_type(fn->return_type_expression());
                fn_spec.spec->set_parameters(fn->parameters());

                if (is_imported)
                {
                    fn_spec.spec->set_codegen([fn = fn_spec.spec.get(), type = fn->return_type_expression()](
                                                  ir_generation_context & ctx) -> codegen::ir::function {
                        auto params = fmap(fn->parameters(), [&](auto && param) {
                            return std::get<std::shared_ptr<codegen::ir::variable>>(
                                param->codegen_ir(ctx).back().result);
                        });

                        auto ret = codegen::ir::function{ std::move(params),
                            codegen::ir::make_variable(fn->return_type_expression()
                                                           ->as<type_expression>()
                                                           ->get_value()
                                                           ->codegen_type(ctx)),
                            {} };
                        ret.is_defined = false;

                        return ret;
                    });
                }
                else
                {
                    function_block_defs.emplace(fn_spec.spec.get(), fn->get_body());
                }

                roset->add_function(fn_spec.spec.get());

                _function_specializations.push_back(std::move(fn_spec));
            }

            roset->resolve_overrides();
        }

        for (auto && fn_spec : _function_specializations)
        {
            auto it = function_block_defs.find(fn_spec.spec.get());
            if (it == function_block_defs.end())
            {
                continue;
            }

            auto body_stmt = repl.copy_claim(it->second);

            auto body_block = dynamic_cast<block *>(body_stmt.get());
            assert(body_block);
            fn_spec.function_body.reset(body_block);
            body_stmt.release();

            fn_spec.spec->set_body(fn_spec.function_body.get());
            fn_spec.spec->set_codegen([this, fn = fn_spec.spec.get()](ir_generation_context & ctx) {
                auto ret =
                    codegen::ir::function{ fmap(fn->parameters(),
                                               [&](auto && param) {
                                                   return std::get<std::shared_ptr<codegen::ir::variable>>(
                                                       param->codegen_ir(ctx).back().result);
                                               }),
                        fn->get_body()->codegen_return(ctx),
                        fn->get_body()->codegen_ir(ctx) };
                ret.is_exported = _exported;
                return ret;
            });
        }
    }

    void typeclass_instance::mark_exported()
    {
        // TODO: make this only get exported when all of the arguments are exported
        // should also be an error to try to export an instance that refers to non-exported arguments

        _exported = true;
        for (auto && oset : _member_overload_set_exprs)
        {
            oset->mark_exported();
        }
    }

    bool typeclass_instance::is_imported() const
    {
        return _imported;
    }

    void typeclass_instance::mark_default()
    {
        _default = true;

        mark_exported();
    }

    bool typeclass_instance::is_default() const
    {
        return _default;
    }

    const typeclass * typeclass_instance::get_typeclass() const
    {
        return _type->get_typeclass();
    }

    void typeclass_instance::print(std::ostream & os, print_context ctx, bool print_members) const
    {
        os << styles::def << ctx << styles::type << "typeclass instance";
        print_address_range(os, this);
        if (_name)
        {
            os << ' ' << styles::string_value << utf8(_name.value());
        }
        os << '\n';

        if (print_members)
        {
            auto rosets_ctx = ctx.make_branch(false);
            os << styles::def << rosets_ctx << styles::subrule_name << "refined overload sets:\n";

            std::size_t idx = 0;
            for (auto && roset : _member_overload_set_exprs)
            {
                roset->get_overload_set()->print(
                    os, rosets_ctx.make_branch(++idx == _member_overload_set_exprs.size()), true);
            }

            auto specs_ctx = ctx.make_branch(true);
            os << styles::def << specs_ctx << styles::subrule_name << "function specializations:\n";

            idx = 0;
            for (auto && spec_info : _function_specializations)
            {
                auto spec_ctx = specs_ctx.make_branch(++idx == _function_specializations.size());
                os << styles::def << spec_ctx << styles::subrule_name << "specialization:\n";

                auto fn_ctx = spec_ctx.make_branch(false);
                os << styles::def << fn_ctx << styles::subrule_name << "function:\n";
                spec_info.spec->print(os, fn_ctx.make_branch(true), true);

                auto body_ctx = spec_ctx.make_branch(true);
                os << styles::def << body_ctx << styles::subrule_name << "body:\n";
                spec_info.function_body->print(os, body_ctx.make_branch(true));
            }
        }
    }

    std::unique_ptr<proto::typeclass_instance> typeclass_instance::generate_interface() const
    {
        auto ret = std::make_unique<proto::typeclass_instance>();

        return ret;
    }
}
}
