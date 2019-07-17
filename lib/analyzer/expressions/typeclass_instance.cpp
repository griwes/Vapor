/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/typeclass_instance.h"

#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/typeclass.h"

#include "expressions/typeclass.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass_instance_expression> preanalyze_instance_literal(precontext & ctx,
        const parser::instance_literal & parse,
        scope * lex_scope)
    {
        auto late_preanalysis = [&parse, &ctx](function_definition_handler fn_def) {
            fmap(parse.definitions, [&](auto && definition) {
                fmap(definition, make_overload_set([&](const parser::function_definition & func) {
                    fn_def(ctx, func);
                    return unit{};
                }));
                return unit{};
            });
        };

        return std::make_unique<typeclass_instance_expression>(
            make_node(parse), std::move(late_preanalysis), make_typeclass_instance(ctx, parse, lex_scope));
    }

    typeclass_instance_expression::typeclass_instance_expression(ast_node parse,
        late_preanalysis_type late_pre,
        std::unique_ptr<typeclass_instance> instance)
        : _late_preanalysis{ std::move(late_pre) }, _instance{ std::move(instance) }
    {
        _set_ast_info(parse);
    }

    typeclass_instance_expression::typeclass_instance_expression(ast_node parse,
        std::unique_ptr<typeclass_instance> instance)
        : _instance{ std::move(instance) }
    {
        _set_ast_info(parse);
    }

    typeclass_instance_expression::~typeclass_instance_expression() = default;

    void typeclass_instance_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "typeclass-instance-literal";
        print_address_range(os, this);
        os << '\n';

        auto tc_ctx = ctx.make_branch(false);
        os << styles::def << tc_ctx << styles::subrule_name << "typeclass:\n";
        _instance->get_typeclass()->print(os, tc_ctx.make_branch(true));

        auto inst_ctx = ctx.make_branch(false);
        os << styles::def << inst_ctx << styles::subrule_name << "defined instance:\n";
        _instance->print(os, inst_ctx.make_branch(true), true);

        auto arg_ctx = ctx.make_branch(_instance->get_member_function_defs().empty());
        os << styles::def << arg_ctx << styles::subrule_name << "arguments:\n";

        std::size_t idx = 0;
        for (auto && arg : _instance->get_arguments())
        {
            arg->print(os, arg_ctx.make_branch(++idx == _instance->get_arguments().size()));
        }

        if (_instance->get_member_function_defs().size())
        {
            auto defn_ctx = ctx.make_branch(true);
            os << styles::def << defn_ctx << styles::subrule_name << "member function definitions:\n";

            std::size_t idx = 0;
            for (auto && member : _instance->get_member_function_defs())
            {
                member->print(
                    os, defn_ctx.make_branch(++idx == _instance->get_member_function_defs().size()));
            }
        }
    }

    expression * typeclass_instance_expression::get_member(const std::u32string & name) const
    {
        return _instance->get_scope()->get(name)->get_expression();
    }

    declaration_ir typeclass_instance_expression::declaration_codegen_ir(ir_generation_context & ctx) const
    {
        auto ret = codegen::ir::make_variable(get_type()->codegen_type(ctx));
        ret->initializer = _constinit_ir(ctx);
        ret->constant = true;
        return { std::move(ret) };
    }

    void typeclass_instance_expression::mark_exported()
    {
        _instance->mark_exported();
    }

    std::unordered_set<expression *> typeclass_instance_expression::get_associated_entities() const
    {
        std::unordered_set<expression *> ret;

        // FIXME: also include the associated entities of the typeclass
        // FIXME: also include arguments
        // FIXME: also include the associated entities of arguments?
        ret.insert(_tc);

        return ret;
    }

    future<typeclass_instance_type *> typeclass_instance_expression::get_instance_type_future() const
    {
        assert(_instance_type_future);
        return _instance_type_future.value();
    }

    typeclass_instance * typeclass_instance_expression::get_instance() const
    {
        return _instance.get();
    }

    void typeclass_instance_expression::_set_name(std::u32string name)
    {
        _instance->set_name(std::move(name));
    }

    future<> typeclass_instance_expression::_analyze(analysis_context & ctx)
    {
        future<typeclass_instance_type *> type = std::get<0>(fmap(_instance->typeclass_reference(),
            make_overload_set(
                [&ctx, this](std::vector<std::u32string> name) {
                    auto top_level = std::move(name.front());
                    name.erase(name.begin());

                    future<expression *> expr =
                        _instance->get_scope()->parent()->resolve(top_level)->get_expression_future();

                    if (!name.empty())
                    {
                        expr = foldl(name, std::move(expr), [&ctx](future<expression *> expr, auto && name) {
                            return expr
                                .then([&](auto && expr) {
                                    return expr->analyze(ctx).then(
                                        [expr] { return expr->get_type()->get_scope(); });
                                })
                                .then([name = std::move(name)](const scope * lex_scope) {
                                    return lex_scope->get(name)->get_expression_future();
                                });
                        });
                    }

                    return expr
                        .then([&](expression * expr) {
                            return expr->analyze(ctx).then([expr] { return expr; });
                        })
                        .then([&](expression * expr) {
                            return when_all(fmap(_instance->get_arguments(),
                                                [&](auto && arg) { return arg->analyze(ctx); }))
                                .then([&] { return _instance->simplify_arguments(ctx); })
                                .then([expr] { return expr; });
                        })
                        .then([&](expression * expr) {
                            _tc = expr->_get_replacement()->as<typeclass_expression>();
                            assert(_tc);

                            return _tc->get_typeclass()->type_for(ctx, _instance->get_argument_values());
                        });
                },
                [&ctx](const imported_type & imported) -> future<typeclass_instance_type *> {
                    return std::get<0>(
                        fmap(imported,
                            make_overload_set(
                                [](class type * resolved) { return make_ready_future(resolved); },

                                [&ctx](std::shared_ptr<unresolved_type> unresolved) {
                                    return unresolved->resolve(ctx).then(
                                        [unresolved] { return unresolved->get_resolved(); });
                                })))
                        .then([](class type * resolved) {
                            auto tc_type = dynamic_cast<typeclass_instance_type *>(resolved);
                            assert(tc_type);
                            return tc_type;
                        });
                })));

        _instance_type_future = type;

        return type.then([&](auto instance_type) {
            _set_type(instance_type);
            _instance->set_type(instance_type);

            if (!_late_preanalysis)
            {
                _instance->import_default_definitions(ctx, true);
                return make_ready_future();
            }

            _late_preanalysis.value()(_instance->get_function_definition_handler());

            return when_all(fmap(_instance->get_member_function_defs(), [&](auto && fn_def) {
                return fn_def->analyze(ctx);
            })).then([&] { _instance->import_default_definitions(ctx); });
        });
    }

    std::unique_ptr<expression> typeclass_instance_expression::_clone_expr(replacements & repl) const
    {
        assert(0);
    }

    future<expression *> typeclass_instance_expression::_simplify_expr(recursive_context ctx)
    {
        return when_all(fmap(_instance->get_member_function_defs(), [&](auto && decl) {
            return decl->simplify(ctx);
        })).then([&](auto &&) -> expression * { return this; });
    }

    constant_init_ir typeclass_instance_expression::_constinit_ir(ir_generation_context & ctx) const
    {
        auto type = get_type()->codegen_type(ctx);
        // TODO: figure out how to get rid of this dynamic pointer cast that is really irritating here
        auto val = codegen::ir::struct_value{ std::dynamic_pointer_cast<codegen::ir::user_type>(type), {} };
        assert(val.type);
        val.fields.reserve(_instance->get_type()->get_overload_sets().size());

        for (auto && [name, base_oset] : _instance->get_type()->get_overload_sets())
        {
            val.fields.push_back(_instance->get_scope()->get(name)->get_expression()->constinit_ir(ctx));
        }

        return val;
    }

    std::unique_ptr<google::protobuf::Message> typeclass_instance_expression::_generate_interface() const
    {
        return _instance->generate_interface();
    }
}
}
