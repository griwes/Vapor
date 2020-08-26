/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/member_assignment.h"
#include "vapor/analyzer/expressions/pack.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    void call_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "call-expression";
        print_address_range(os, this);
        os << '\n';

        if (_replacement_expr)
        {
            auto replacement_ctx = ctx.make_branch(false);

            os << styles::def << replacement_ctx << styles::subrule_name << "replacement expression:\n";
            _replacement_expr->print(os, replacement_ctx.make_branch(true));
        }

        auto type_ctx = ctx.make_branch(false);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        get_type()->print(os, type_ctx.make_branch(true));

        auto function_ctx = ctx.make_branch(_args.empty() && !_vtable_arg);
        os << styles::def << function_ctx << styles::subrule_name << "function:\n";
        _function->print(os, function_ctx.make_branch(true));

        if (_vtable_arg)
        {
            auto vtable_arg_ctx = ctx.make_branch(_args.empty());
            os << styles::def << vtable_arg_ctx << styles::subrule_name << "vtable argument:\n";
            _vtable_arg->print(os, vtable_arg_ctx.make_branch(true));
        }

        if (_args.size())
        {
            auto args_ctx = ctx.make_branch(true);
            os << styles::def << args_ctx << styles::subrule_name << "arguments:\n";

            std::size_t idx = 0;
            for (auto && arg : _args)
            {
                arg->print(os, args_ctx.make_branch(++idx == _args.size()));
            }
        }
    }

    future<> call_expression::_analyze(analysis_context & ctx)
    {
        return _function->run_analysis_hooks(ctx, this, _args).then([&]() {
            if (_replacement_expr)
            {
                return _replacement_expr->analyze(ctx).then(
                    [&] { this->_set_type(_replacement_expr->get_type()); });
            }

            return _function->get_return_type().then([&](expression * type_expr) {
                assert(type_expr);
                assert(type_expr->get_type() == builtin_types().type);

                if (!type_expr->is_constant())
                {
                    replacements repl;
                    std::vector<std::shared_ptr<expression>> var_space;
                    std::vector<std::shared_ptr<expression>> expr_space = fmap(_args, [&](auto && arg) {
                        std::shared_ptr<expression> cloned = repl.claim(arg);
                        return cloned;
                    });

                    auto arg_begin = _args.begin();
                    auto arg_end = _args.end();
                    auto param_begin = _function->parameters().begin();
                    auto param_end = _function->parameters().end();

                    std::vector<type *> matching_space;
                    std::vector<expression *> matching_expressions;
                    matching_space.reserve(arg_end - arg_begin);
                    matching_expressions.reserve(arg_end - arg_begin);

                    while (arg_begin != arg_end && param_begin != param_end)
                    {
                        matching_space.clear();
                        matching_expressions.clear();

                        auto && param_type = (*param_begin)->get_type();

                        if (!param_type->matches((*arg_begin)->get_type()))
                        {
                            if (!param_type->matches(matching_space))
                            {
                                assert(0);
                            }

                            auto empty_pack = make_pack_expression();
                            repl.add_replacement(*param_begin, empty_pack.get());
                            var_space.push_back(std::move(empty_pack));

                            ++param_begin;
                            continue;
                        }

                        bool last_matched;

                        do
                        {
                            matching_space.push_back((*arg_begin)->get_type());
                            matching_expressions.push_back(*arg_begin);
                        } while (
                            (last_matched = param_type->matches(matching_space)) && ++arg_begin != arg_end);

                        if (matching_space.size() == 1 && !last_matched)
                        {
                            if (*param_begin == type_expr)
                            {
                                // this is the simple case: one of the arguments is literally returned
                                // AND the function actually provides this information the way it should
                                // this is a special case that is PRIMARILY here for the generic constructor
                                //
                                // the other way does work for it too, but... it's not pretty resource-wise
                                auto expr = matching_expressions.front();
                                assert(expr->get_type() == builtin_types().type);

                                auto type_expr = expr->as<type_expression>();
                                assert(type_expr);
                                assert(type_expr->get_value() != builtin_types().type);

                                this->_set_type(type_expr->get_value());

                                return make_ready_future();
                            }

                            var_space.push_back(repl.claim(matching_expressions.front()));

                            ++arg_begin;
                            ++param_begin;
                            continue;
                        }

                        auto pack = make_pack_expression(
                            fmap(matching_expressions, [&](auto && var) { return repl.claim(var); }),
                            (*param_begin)->get_type());
                        repl.add_replacement(*param_begin, pack.get());
                        var_space.push_back(std::move(pack));

                        ++param_begin;
                    }

                    assert(arg_begin == arg_end);
                    assert(param_begin == param_end);

                    _cloned_type_expr = repl.claim(type_expr);

                    return simplification_loop(ctx, _cloned_type_expr)
                        .then([this, var_space, expr_space](expression * type_expr) {
                            assert(type_expr);
                            assert(type_expr->get_type() == builtin_types().type);

                            auto expr = type_expr->as<type_expression>();
                            assert(expr);
                            assert(expr->get_value() != builtin_types().type);

                            this->_set_type(expr->get_value());
                        });
                }

                auto expr = type_expr->as<type_expression>();
                assert(expr->get_value() != builtin_types().type);

                this->_set_type(expr->get_value());

                return make_ready_future();
            });
        });
    }

    std::unique_ptr<expression> call_expression::_clone_expr(replacements & repl) const
    {
        if (_replacement_expr)
        {
            return repl.claim(_replacement_expr.get());
        }

        auto fn = repl.try_get_replacement(_function);
        if (!fn)
        {
            fn = _function;
        }

        std::unique_ptr<expression> vtable_arg;
        if (_vtable_arg && fn->vtable_slot())
        {
            vtable_arg = repl.claim(_vtable_arg.get());
        }

        auto ret = std::make_unique<owning_call_expression>(fn,
            std::move(vtable_arg),
            fmap(_args, [&](auto arg) { return repl.copy_claim(arg); }),
            get_scope(),
            get_name());

        ret->set_ast_info(get_ast_info().value());

        if (_cloned_type_expr)
        {
            ret->_cloned_type_expr = repl.claim(_cloned_type_expr.get());
            auto type_expr = ret->_cloned_type_expr->as<type_expression>();
            assert(type_expr);

            auto type = type_expr->get_type();
            if (auto new_type = repl.try_get_replacement(type))
            {
                type = new_type;
            }
            ret->_set_type(type);
            return ret;
        }

        auto type = repl.try_get_replacement(get_type());
        if (!type)
        {
            type = get_type();
        }

        ret->_set_type(type);
        return ret;
    }

    future<expression *> call_expression::_simplify_expr(recursive_context ctx)
    {
        if (_replacement_expr)
        {
            return make_ready_future(_replacement_expr.release());
        }

        auto vtable_arg_future = make_ready_future();

        if (_vtable_arg)
        {
            vtable_arg_future = _vtable_arg->simplify_expr(ctx).then(
                [&, ctx](auto && expr) { replace_uptr(_vtable_arg, expr, ctx.proper); });
        }

        return vtable_arg_future
            .then([&, ctx] {
                if (_vtable_arg)
                {
                    assert(_function->vtable_slot());
                    auto repl_fn =
                        _vtable_arg->_get_replacement()->get_vtable_entry(_function->vtable_slot().value());
                    if (repl_fn)
                    {
                        _function = repl_fn;
                        _vtable_arg.reset();
                        ctx.proper.something_happened();
                    }
                }

                return when_all(fmap(_args, [&](auto && arg) { return arg->simplify_expr(ctx); }));
            })
            .then([&, ctx](auto && repl) {
                assert(_args.size() == repl.size());
                for (std::size_t i = 0; i < _args.size(); ++i)
                {
                    if (repl[i] && repl[i] != _args[i])
                    {
                        _args[i] = repl[i];
                        ctx.proper.something_happened();
                    }
                }

                logger::dlog(logger::trace) << "Simplifying call_expr " << this;
                return _function->simplify(ctx, this, _args);
            });
    }

    future<expression *> owning_call_expression::_simplify_expr(recursive_context ctx)
    {
        if (_replacement_expr)
        {
            return _replacement_expr->simplify_expr(ctx).then([&, ctx](auto && repl) -> expression * {
                replace_uptr(_replacement_expr, repl, ctx.proper);
                return this;
            });
        }

        return when_all(fmap(_var_exprs, [&](auto && arg) {
            return arg->simplify_expr(ctx);
        })).then([&, ctx](auto && repl) {
            replace_uptrs(_var_exprs, repl, ctx.proper);
            return this->call_expression::_simplify_expr(ctx);
        });
    }

    statement_ir call_expression::_codegen_ir(ir_generation_context & ctx) const
    {
        if (_replacement_expr)
        {
            return _replacement_expr->codegen_ir(ctx);
        }

        if (_function->is_member())
        {
            ctx.push_base_expression(_args.front());
        }

        statement_ir ret;

        auto arguments_instructions = fmap(_args, [&](auto && arg) { return arg->codegen_ir(ctx); });
        auto arguments_values =
            fmap(arguments_instructions, [](auto && insts) { return insts.back().result; });

        if (_function->vtable_slot())
        {
            auto vtable_argument_ir = _vtable_arg->codegen_ir(ctx);
            auto vtable = vtable_argument_ir.back().result;
            std::move(vtable_argument_ir.begin(), vtable_argument_ir.end(), std::back_inserter(ret));

            // TODO/FIXME: this should not operate on values, but on pointers; simiarly, passing an instance
            // should pass the vtable by pointer, not by value
            auto call_operand = codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::member_access_instruction>() },
                { vtable, codegen::ir::integer_value{ _function->vtable_slot().value() } },
                { codegen::ir::make_variable(
                    codegen::ir::builtin_types().function(get_type()->codegen_type(ctx),
                        fmap(_args, [&](auto && arg) { return arg->get_type()->codegen_type(ctx); }))) } };

            arguments_values.insert(arguments_values.begin(), call_operand.result);

            ret.push_back(std::move(call_operand));
        }

        fmap(arguments_instructions, [&](auto && insts) {
            std::move(insts.begin(), insts.end(), std::back_inserter(ret));
            return unit{};
        });

        if (_function->is_intrinsic())
        {
            auto intrinsic_instructions = _function->intrinsic_codegen_ir(ctx, std::move(arguments_values));
            std::move(intrinsic_instructions.begin(), intrinsic_instructions.end(), std::back_inserter(ret));
        }
        else
        {
            if (!_function->vtable_slot())
            {
                arguments_values.insert(arguments_values.begin(), _function->pointer_ir(ctx));
            }

            if (_function->is_member())
            {
                assert(arguments_values.size() >= 2);
                std::swap(arguments_values[0], arguments_values[1]);
            }

            auto call_expr_instruction = codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::function_call_instruction>() },
                std::move(arguments_values),
                { codegen::ir::make_variable(get_type()->codegen_type(ctx)) } };
            ret.push_back(std::move(call_expr_instruction));
        }

        if (_function->is_member())
        {
            ctx.pop_base_expression(_args.front());
        }

        return ret;
    }

    constant_init_ir call_expression::_constinit_ir(ir_generation_context &) const
    {
        assert(0);
    }
}
}
