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

#include <numeric>

#include <reaver/prelude/fold.h>

#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/block.h"
#include "vapor/analyzer/statements/return.h"
#include "vapor/parser.h"
#include "vapor/utf.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<block> preanalyze_block(precontext & ctx,
        const parser::block & parse,
        scope * lex_scope,
        bool is_top_level)
    {
        auto scope = lex_scope->clone_for_local();

        auto statements = fmap(parse.block_value, [&](auto && row) {
            return std::get<0>(fmap(row,
                make_overload_set(
                    [&](const parser::block & block) -> std::unique_ptr<statement> {
                        return preanalyze_block(ctx, block, scope.get(), false);
                    },
                    [&](const parser::statement & statement) {
                        auto scope_ptr = scope.get();
                        auto ret = preanalyze_statement(ctx, statement, scope_ptr);
                        if (scope_ptr != scope.get())
                        {
                            scope.release()->keep_alive();
                            scope.reset(scope_ptr);
                        }
                        return ret;
                    })));
        });

        scope->close();

        auto scope_ptr = scope.get();
        return std::make_unique<block>(make_node(parse),
            std::move(scope),
            lex_scope,
            std::move(statements),
            fmap(parse.value_expression,
                [&](auto && val_expr) { return preanalyze_expression(ctx, val_expr, scope_ptr); }),
            is_top_level);
    }

    block::block(ast_node parse,
        std::unique_ptr<scope> lex_scope,
        scope * original_scope,
        std::vector<std::unique_ptr<statement>> statements,
        std::optional<std::unique_ptr<expression>> value_expr,
        bool is_top_level)
        : _scope{ std::move(lex_scope) },
          _original_scope{ original_scope },
          _statements{ std::move(statements) },
          _value_expr{ std::move(value_expr) },
          _is_top_level{ is_top_level }
    {
        _set_ast_info(parse);
    }

    type * block::return_type() const
    {
        auto return_types = fmap(get_returns(), [](auto && stmt) { return stmt->get_returned_type(); });

        auto val = value_type();
        if (val)
        {
            return_types.push_back(val);
        }

        std::sort(return_types.begin(), return_types.end());
        return_types.erase(std::unique(return_types.begin(), return_types.end()), return_types.end());
        assert(return_types.size() == 1);
        return return_types.front();
    }

    void block::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "block";
        print_address_range(os, this);
        os << '\n';

        auto stmts_ctx = ctx.make_branch(!_value_expr);
        os << styles::def << stmts_ctx << styles::subrule_name << "statements:\n";

        std::size_t idx = 0;
        for (auto && stmt : _statements)
        {
            stmt->print(os, stmts_ctx.make_branch(++idx == _statements.size()));
        }

        if (_value_expr)
        {
            auto value_ctx = ctx.make_branch(true);
            os << styles::def << value_ctx << styles::subrule_name << "value expression:\n";

            _value_expr.value()->print(os, value_ctx.make_branch(true));
        }
    }

    future<> block::_analyze(analysis_context & ctx)
    {
        auto fut = when_all(fmap(_statements, [&](auto && stmt) { return stmt->analyze(ctx); }));

        fmap(_value_expr, [&](auto && expr) {
            fut = fut.then([&ctx, expr = expr.get()] { return expr->analyze(ctx); });
            return unit{};
        });

        return fut;
    }

    std::unique_ptr<statement> block::_clone(replacements & repl) const
    {
        auto ret = std::unique_ptr<block>(new block(*this));

        ret->_statements = fmap(_statements, [&](auto && stmt) { return repl.claim(stmt.get()); });
        ret->_value_expr = fmap(_value_expr, [&](auto && expr) { return repl.claim(expr.get()); });

        return ret;
    }

    future<statement *> block::_simplify(recursive_context ctx)
    {
        if (!_value_expr && _statements.size() == 1)
        {
            return _statements.front()->simplify(ctx).then([&, ctx](auto && simpl) -> statement * {
                replace_uptr(_statements.front(), simpl, ctx.proper);

                if (_is_top_level)
                {
                    return this;
                }
                return _statements.front().release();
            });
        }

        auto fut = foldl(_statements, make_ready_future(true), [&](auto future, auto && statement) {
            return future.then([&, ctx](bool do_continue) {
                if (!do_continue)
                {
                    return make_ready_future(false);
                }

                return statement->simplify(ctx).then([&, ctx](auto && simplified) {
                    replace_uptr(statement, simplified, ctx.proper);
                    return !statement->always_returns();
                });
            });
        });

        fmap(_value_expr, [&](auto && expr) {
            fut = fut.then([&, expr = expr.get(), ctx](bool do_continue) {
                if (!do_continue)
                {
                    return make_ready_future(false);
                }

                return expr->simplify_expr(ctx).then([&, ctx](auto && simplified) {
                    replace_uptr(*_value_expr, simplified, ctx.proper);
                    return true;
                });
            });
            return unit{};
        });

        return fut.then([&](bool reached_end) -> statement * {
            if (!reached_end)
            {
                auto always_returning = std::find_if(_statements.begin(),
                    _statements.end(),
                    [](auto && stmt) { return stmt->always_returns(); });

                assert(always_returning != _statements.end());
                _statements.erase(always_returning + 1, _statements.end());
            }

            return this;
        });
    }

    std::vector<codegen::_v1::ir::instruction> block::_codegen_ir(ir_generation_context & ctx) const
    {
        auto statements = mbind(_statements, [&](auto && stmt) { return stmt->codegen_ir(ctx); });
        fmap(_value_expr, [&](auto && expr) {
            auto instructions = expr->codegen_ir(ctx);
            instructions.emplace_back(codegen::ir::instruction{ std::nullopt,
                std::nullopt,
                { boost::typeindex::type_id<codegen::ir::return_instruction>() },
                {},
                instructions.back().result });
            std::move(instructions.begin(), instructions.end(), std::back_inserter(statements));
            return unit{};
        });

        statement_ir scope_cleanup;
        // FIXME: actually implement destructors in a non-retarded manner
        /*for (auto scope = _scope.get(); scope != _original_scope->parent(); scope = scope->parent())
        {
            std::transform(scope->symbols_in_order().rbegin(), scope->symbols_in_order().rend(),
        std::back_inserter(scope_cleanup), [&ctx](auto && symbol) { auto ir =
        symbol->get_expression()->codegen_ir(ctx).back().result; return codegen::ir::instruction{ {}, {}, {
        boost::typeindex::type_id<codegen::ir::destruction_instruction>() }, { ir }, ir };
            });
        }*/

        for (std::size_t i = 0; i < statements.size(); ++i)
        {
            auto & stmt = statements[i];
            if (!stmt.instruction.template is<codegen::ir::return_instruction>())
            {
                continue;
            }

            statements.insert(statements.begin() + i, scope_cleanup.begin(), scope_cleanup.end());
            i += scope_cleanup.size();
        }

        if (_is_top_level)
        {
            std::vector<codegen::ir::value> labeled_return_values;
            std::u32string current_label = U"entry";

            for (std::size_t i = 0; i < statements.size(); ++i)
            {
                auto & stmt = statements[i];

                if (stmt.label)
                {
                    current_label = *stmt.label;
                }

                if (!stmt.instruction.template is<codegen::ir::return_instruction>())
                {
                    continue;
                }

                std::u32string label;
                if (!stmt.label)
                {
                    label = current_label;
                }
                else
                {
                    label = stmt.label.value();
                }

                labeled_return_values.emplace_back(codegen::ir::label{ label });
                labeled_return_values.emplace_back(stmt.result);
            }

            // in this case we don't generate jumps
            // if we generate the unnecessary label, it'll turn into an LLVM block
            // and LLVM blocks need to have a branch before them
            // hence, remove the label if it's useless
            if (labeled_return_values.size() > 2)
            {
                std::size_t return_value_index = 0;

                for (std::size_t i = 0; i < statements.size(); ++i)
                {
                    auto & stmt = statements[i];

                    if (!stmt.instruction.template is<codegen::ir::return_instruction>())
                    {
                        continue;
                    }

                    stmt.instruction = boost::typeindex::type_id<codegen::ir::jump_instruction>();
                    stmt.operands = { codegen::ir::label{ U"return_phi" } };

                    // create a variable for the constant return value
                    if (stmt.result.index() != 0)
                    {
                        auto old_result = stmt.result;
                        auto var = make_variable(get_type(old_result));
                        stmt.result = var;
                        statements.insert(statements.begin() + i++,
                            { {},
                                {},
                                { boost::typeindex::type_id<codegen::ir::materialization_instruction>() },
                                { old_result },
                                var });
                        labeled_return_values[2 * return_value_index + 1] = var;
                    }

                    ++return_value_index;
                }

                statements.emplace_back(
                    codegen::ir::instruction{ std::optional<std::u32string>{ U"return_phi" },
                        std::nullopt,
                        { boost::typeindex::type_id<codegen::ir::phi_instruction>() },
                        std::move(labeled_return_values),
                        codegen::ir::make_variable(return_type()->codegen_type(ctx)) });

                statements.emplace_back(codegen::ir::instruction{ std::nullopt,
                    std::nullopt,
                    { boost::typeindex::type_id<codegen::ir::return_instruction>() },
                    {},
                    statements.back().result });
            }
        }

        return statements;
    }

    codegen::_v1::ir::value block::codegen_return(ir_generation_context & ctx) const
    {
        assert(_is_top_level);
        return codegen_ir(ctx).back().result;
    }
}
}
