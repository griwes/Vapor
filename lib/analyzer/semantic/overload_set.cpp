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

#include "vapor/analyzer/semantic/overload_set.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/function.h"

#include "types/overload_set.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct imported_function
    {
        std::unique_ptr<expression> return_type;
        std::vector<std::unique_ptr<expression>> parameters;
        std::unique_ptr<class function> function;
    };

    std::unique_ptr<overload_set> import_overload_set(precontext & ctx, const proto::overload_set_type & type)
    {
        auto ret = std::make_unique<overload_set>(ctx.current_lex_scope->clone_for_type(
            utf32(ctx.current_symbol).substr(ctx.current_lex_scope->get_scoped_name().size())));

        assert(type.functions_size() != 0);

        for (auto && overload : type.functions())
        {
            auto imported = std::make_unique<imported_function>();
            imported->return_type = get_imported_type_ref_expr(ctx, overload.return_type());

            for (auto && param : overload.parameters())
            {
                auto parm = std::make_unique<parameter>(imported_ast_node(ctx, param.range()),
                    ctx.current_lex_scope,
                    utf32(param.name()),
                    get_imported_type_ref_expr(ctx, param.type()));
                imported->parameters.push_back(std::move(parm));
            }

            imported->function = make_function("overloadable function");
            imported->function->set_return_type(imported->return_type.get());
            imported->function->set_parameters(
                fmap(imported->parameters, [](auto && param) { return param.get(); }));
            imported->function->set_codegen(
                [imported = imported.get()](ir_generation_context & ctx) -> codegen::ir::function {
                    auto params = fmap(imported->parameters, [&](auto && param) {
                        return std::get<std::shared_ptr<codegen::ir::variable>>(
                            param->codegen_ir(ctx).back().result);
                    });

                    auto ret = codegen::ir::function{ std::move(params),
                        codegen::ir::make_variable(
                            imported->return_type->as<type_expression>()->get_value()->codegen_type(ctx)),
                        {} };
                    ret.is_member = true;
                    ret.is_defined = false;

                    return ret;
                });

            if (overload.is_member())
            {
                imported->function->make_member();
            }

            if (overload.has_vtable_id())
            {
                imported->function->mark_virtual(overload.vtable_id());
            }

            imported->function->set_name(ret->get_type()->get_scope()->get_entity_name() + U".call");
            ret->add_function(std::move(imported));
        }

        return ret;
    }

    void overload_set_base::add_function(function * fn)
    {
        if (std::find_if(_functions.begin(),
                _functions.end(),
                [&](auto && f) { return f->parameters() == fn->parameters(); })
            != _functions.end())
        {
            assert(0);
        }

        _functions.push_back(fn);
    }

    void overload_set_base::add_function(std::unique_ptr<imported_function> fn)
    {
        add_function(fn->function.get());
        _imported_functions.push_back(std::move(fn));
    }

    overload_set::overload_set(std::unique_ptr<scope> lex_scope)
        : _type{ std::make_unique<overload_set_type>(std::move(lex_scope), this) }
    {
    }

    std::vector<function *> overload_set::get_overloads() const
    {
        return _functions;
    }

    overload_set_type * overload_set::get_type() const
    {
        return _type.get();
    }

    void overload_set::print(std::ostream & os, print_context ctx, bool print_members) const
    {
        os << styles::def << ctx << styles::rule_name << "overload set" << styles::def << " @ "
           << styles::address << this << styles::def << ":\n";

        auto type_ctx = ctx.make_branch(_functions.empty() || !print_members);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        _type->print(os, type_ctx.make_branch(true));

        if (!_functions.empty() && print_members)
        {
            auto members_ctx = ctx.make_branch(true);
            os << styles::def << members_ctx << styles::subrule_name << "functions:\n";

            std::size_t idx = 0;
            for (auto && fn : _functions)
            {
                fn->print(os, members_ctx.make_branch(++idx == _functions.size()), true);
            }
        }
    }

    refined_overload_set::refined_overload_set(scope * lex_scope, overload_set * base)
        : _base{ base },
          _refined_scope{ lex_scope->clone_for_type(std::u32string{ base->get_scope()->get_name() }) }
    {
    }

    std::vector<function *> refined_overload_set::get_overloads() const
    {
        std::vector<function *> ret;
        ret.reserve(_functions.size() + _vtable_entries.size());
        ret = _functions;
        for (auto && [idx, fn] : _vtable_entries)
        {
            ret.push_back(fn);
        }
        return ret;
    }

    overload_set_type * refined_overload_set::get_type() const
    {
        return _base->get_type();
    }

    void refined_overload_set::resolve_overrides()
    {
        auto base_overloads = _base->get_overloads();

        while (!_functions.empty())
        {
            auto fn = _functions.back();

            auto orig_it = std::find_if(base_overloads.begin(), base_overloads.end(), [&](function * orig) {
                auto fn_ret_type_expr = fn->return_type_expression()->as<type_expression>();
                auto orig_ret_type_expr = orig->return_type_expression()->as<type_expression>();

                assert(fn_ret_type_expr && orig_ret_type_expr);

                if (fn_ret_type_expr->get_value() != orig_ret_type_expr->get_value())
                {
                    return false;
                }

                return std::equal(fn->parameters().begin(),
                    fn->parameters().end(),
                    orig->parameters().begin(),
                    orig->parameters().end(),
                    [&](auto && fn_param, auto && orig_param) {
                        return fn_param->get_type() == orig_param->get_type();
                    });
            });

            if (orig_it == base_overloads.end())
            {
                assert(0);
            }

            auto orig = *orig_it;
            auto slot = orig->vtable_slot();
            assert(slot);

            auto result = _vtable_entries.emplace(slot.value(), fn);
            assert(result.second);

            base_overloads.erase(orig_it);
            _functions.pop_back();
        }
    }

    void refined_overload_set::verify_overrides() const
    {
        for (auto && overload : _base->get_overloads())
        {
            // TODO: turn this into actual error reporting
            assert(get_vtable_entry(overload->vtable_slot().value()));
        }
    }

    function * refined_overload_set::get_vtable_entry(std::size_t vtable_id) const
    {
        auto it = _vtable_entries.find(vtable_id);
        if (it != _vtable_entries.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void refined_overload_set::print(std::ostream & os, print_context ctx, bool print_members) const
    {
        os << styles::def << ctx << styles::rule_name << "overload set" << styles::def << " @ "
           << styles::address << this << styles::def << ":\n";

        auto functions = get_overloads();

        auto type_ctx = ctx.make_branch(functions.empty() || !print_members);
        os << styles::def << type_ctx << styles::subrule_name << "type:\n";
        _base->get_type()->print(os, type_ctx.make_branch(true));

        if (!functions.empty() && print_members)
        {
            auto members_ctx = ctx.make_branch(true);
            os << styles::def << members_ctx << styles::subrule_name << "functions:\n";

            std::size_t idx = 0;
            for (auto && fn : functions)
            {
                fn->print(os, members_ctx.make_branch(++idx == functions.size()), true);
            }
        }
    }
}
}
