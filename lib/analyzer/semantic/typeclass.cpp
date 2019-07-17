/**
 * Vapor Compiler Licence
 *
 * Copyright © 2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/semantic/typeclass.h"

#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/parameter_list.h"
#include "vapor/analyzer/statements/function.h"
#include "vapor/analyzer/types/typeclass_instance.h"
#include "vapor/parser/expr.h"
#include "vapor/parser/typeclass.h"

#include "expressions/type.pb.h"
#include "expressions/typeclass.pb.h"
#include "type_reference.pb.h"
#include "types/overload_set.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<typeclass> make_typeclass(precontext & ctx,
        const parser::typeclass_literal & parse,
        scope * lex_scope)
    {
        auto scope = lex_scope->clone_for_class();
        auto scope_ptr = scope.get();

        auto params = preanalyze_parameter_list(ctx, parse.parameters, scope_ptr);

        std::vector<std::unique_ptr<function_declaration>> fn_decls;

        fmap(parse.members, [&](auto && member) {
            fmap(member,
                make_overload_set(
                    [&](const parser::function_declaration & decl) {
                        fn_decls.push_back(preanalyze_function_declaration(ctx, decl, scope_ptr));
                        return unit{};
                    },
                    [&](const parser::function_definition & def) {
                        fn_decls.push_back(preanalyze_function_definition(ctx, def, scope_ptr));
                        return unit{};
                    }));

            return unit{};
        });

        scope_ptr->close();

        std::unordered_map<std::u32string, std::size_t> overload_set_function_counts;
        for (auto && fn_decl : fn_decls)
        {
            fn_decl->get_function()->mark_virtual(overload_set_function_counts[fn_decl->get_name()]++);
        }

        return std::make_unique<typeclass>(
            make_node(parse), std::move(scope), std::move(params), std::move(fn_decls));
    }

    std::unique_ptr<typeclass> import_typeclass(precontext & ctx, const proto::typeclass & tc)
    {
        auto tc_scope = ctx.current_lex_scope->clone_for_class();
        auto old_scope = std::exchange(ctx.current_lex_scope, tc_scope.get());

        std::vector<std::unique_ptr<parameter>> params;
        params.reserve(tc.parameters().size());
        for (auto && param : tc.parameters())
        {
            auto parm = std::make_unique<parameter>(imported_ast_node(ctx, param.range()),
                utf32(param.name()),
                get_imported_type_ref_expr(ctx, param.type()));
            tc_scope->init(utf32(param.name()), make_symbol(utf32(param.name()), parm.get()));
            params.push_back(std::move(parm));
        }

        std::vector<std::unique_ptr<expression>> keepalive;
        keepalive.reserve(tc.overload_sets().size());
        // for some reason clang-7 explodes when this loop below uses structured bindings. wut?
        // probably related with the fact that clang-7 based YCM thinks that proto::typeclas is incomplete in
        // this file, which is also unhinged?
        // trying to creduce the ICE, but it's going slowly, so to be able
        // to work in the meantime, we'll just avoid using structured bindings here for now
        for (auto && overset : tc.overload_sets())
        {
            auto oset = import_overload_set(ctx, overset.second);
            auto expr = std::make_unique<overload_set_expression>(std::move(oset));
            tc_scope->init(utf32(overset.first), make_symbol(utf32(overset.first), expr.get()));
            keepalive.push_back(std::move(expr));
        }

        tc_scope->close();
        ctx.current_lex_scope = old_scope;

        return std::make_unique<typeclass>(
            imported_ast_node(ctx, tc.range()), std::move(tc_scope), std::move(params), std::move(keepalive));
    }

    typeclass::typeclass(ast_node parse,
        std::unique_ptr<scope> member_scope,
        std::vector<std::unique_ptr<parameter>> parameters,
        std::vector<std::unique_ptr<function_declaration>> member_function_decls)
        : _parse{ parse },
          _scope{ std::move(member_scope) },
          _parameters{ std::move(parameters) },
          _member_function_declarations{ std::move(member_function_decls) }
    {
    }

    typeclass::typeclass(ast_node parse,
        std::unique_ptr<scope> member_scope,
        std::vector<std::unique_ptr<parameter>> parameters,
        std::vector<std::unique_ptr<expression>> keepalive)
        : _parse{ parse },
          _scope{ std::move(member_scope) },
          _parameters{ std::move(parameters) },
          _keepalive{ std::move(keepalive) }
    {
    }

    typeclass::~typeclass() = default;

    void typeclass::print(std::ostream & os, print_context ctx, bool print_members) const
    {
        os << styles::def << ctx << styles::type << "typeclass";
        print_address_range(os, this);
        if (_name)
        {
            os << ' ' << styles::string_value << utf8(_name.value());
        }
        os << '\n';

        if (print_members)
        {
            auto osets_ctx = ctx.make_branch(true);
            os << styles::def << osets_ctx << styles::subrule_name << "overload sets:\n";

            std::vector<overload_set *> osets;
            for (auto && symbol : _scope->symbols_in_order())
            {
                if (auto oset = symbol->get_expression()->as<overload_set_expression>())
                {
                    osets.push_back(oset->get_overload_set());
                }
            }

            std::size_t idx = 0;
            for (auto && oset : osets)
            {
                oset->print(os, osets_ctx.make_branch(++idx == osets.size()), true);
            }
        }
    }

    std::vector<parameter *> typeclass::get_parameters() const
    {
        return fmap(_parameters, [](auto && param) { return param.get(); });
    }

    std::vector<expression *> typeclass::get_parameter_expressions() const
    {
        return fmap(_parameters, [](auto && param) -> expression * { return param.get(); });
    }

    future<typeclass_instance_type *> typeclass::type_for(analysis_context & ctx,
        const std::vector<type *> & arguments)
    {
        auto & type_info = _instance_types[arguments];
        if (!type_info.instance)
        {
            type_info.instance = std::make_unique<typeclass_instance_type>(this, arguments);
            type_info.analysis_future =
                type_info.instance->_analyze(ctx).then([i = type_info.instance.get()] { return i; });
        }

        return type_info.analysis_future.value();
    }

    void typeclass::set_name(std::u32string name)
    {
        assert(!_name);
        _name = std::move(name);
    }

    std::u32string typeclass::codegen_name(ir_generation_context &) const
    {
        assert(_name || !"typeclasses with no name bound in the frontend are not supported yet!");
        return _name.value();
    }

    std::unique_ptr<proto::typeclass> typeclass::generate_interface() const
    {
        auto ret = std::make_unique<proto::typeclass>();

        for (auto && param : _parameters)
        {
            auto parm = ret->add_parameters();
            parm->set_name(utf8(param->get_name()));
            parm->set_allocated_type(param->get_type()->generate_interface_reference().release());
        }

        auto & mut_osets = *ret->mutable_overload_sets();
        for (auto && symbol : _scope->symbols_in_order())
        {
            if (auto oset = symbol->get_expression()->as<overload_set_expression>())
            {
                auto interface = oset->get_type()->generate_interface();
                assert(interface->has_overload_set());
                mut_osets[utf8(symbol->get_name())] = interface->overload_set();
            }
        }

        return ret;
    }

    std::unique_ptr<proto::user_defined_reference> typeclass::generate_interface_reference() const
    {
        auto user_defined = std::make_unique<proto::user_defined_reference>();

        for (auto scope : get_scope()->codegen_ir())
        {
            switch (scope.type)
            {
                case codegen::ir::scope_type::module:
                    *user_defined->add_module() = utf8(scope.name);
                    break;

                case codegen::ir::scope_type::type:
                    assert(!"should probably implement nested UDT references now... ;)");

                default:
                    assert(0);
            }
        }

        assert(_name);
        user_defined->set_name(utf8(_name.value()));

        return user_defined;
    }
}
}

