/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/types/struct.h"
#include "vapor/analyzer/expressions/variable.h"
#include "vapor/analyzer/statements/declaration.h"
#include "vapor/analyzer/symbol.h"
#include "vapor/analyzer/variables/struct.h"
#include "vapor/parser/expr.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct_type::~struct_type() = default;

    struct_type::struct_type(const parser::struct_literal & parse, scope * lex_scope) : type{ lex_scope }, _parse{ parse }
    {
        auto pair = make_promise<function *>();
        _aggregate_ctor_future = std::move(pair.future);
        _aggregate_ctor_promise = std::move(pair.promise);

        fmap(parse.members, [&](auto && member) {
            fmap(member,
                make_overload_set(
                    [&](const parser::declaration & decl) {
                        auto scope = _member_scope.get();
                        auto decl_stmt = preanalyze_member_declaration(decl, scope);
                        assert(scope == _member_scope.get());
                        _data_members_declarations.push_back(std::move(decl_stmt));

                        return unit{};
                    },

                    [&](const parser::function & func) {
                        assert(0);
                        return unit{};
                    }));

            return unit{};
        });

        _member_scope->close();
    }

    future<function *> struct_type::get_constructor(std::vector<const variable *> args) const
    {
        return _aggregate_ctor_future->then([&, args = std::move(args)](auto ret) {
            if (args.size() != _data_members.size())
            {
                if (args.size() > _data_members.size())
                {
                    ret = nullptr;
                    return ret;
                }

                if (!std::all_of(_data_members.begin() + args.size(), _data_members.end(), [](auto && arg) { return arg->get_default_value() != nullptr; }))
                {
                    ret = nullptr;
                    return ret;
                }
            }

            if (!std::equal(args.begin(), args.end(), _data_members.begin(), [](auto && arg, auto && member) { return arg->get_type() == member->get_type(); }))
            {
                ret = nullptr;
                return ret;
            }

            return ret;
        });
    }

    void struct_type::generate_constructor()
    {
        _data_members =
            fmap(_data_members_declarations, [&](auto && member) { return static_cast<member_variable *>(member->declared_symbol()->get_variable()); });

        _aggregate_ctor =
            make_function("struct type constructor", this, fmap(_data_members, [](auto && member) -> variable * { return member; }), [&](auto && ctx) {
                auto ir_type = this->codegen_type(ctx);
                auto args = fmap(_data_members, [&](auto && member) { return get<codegen::ir::value>(member->codegen_ir(ctx)); });
                auto result = codegen::ir::make_variable(ir_type);

                auto aggregate_ctor = codegen::ir::function{ U"constructor",
                    get_scope()->codegen_ir(ctx),
                    fmap(args, [](auto && arg) { return get<std::shared_ptr<codegen::ir::variable>>(arg); }),
                    result,
                    { codegen::ir::instruction{ none, none, { boost::typeindex::type_id<codegen::ir::aggregate_init_instruction>() }, args, result },
                        codegen::ir::instruction{ none, none, { boost::typeindex::type_id<codegen::ir::return_instruction>() }, { result }, result } } };
                aggregate_ctor.scopes = get_scope()->codegen_ir(ctx);
                aggregate_ctor.scopes.emplace_back(_codegen_type_name(ctx), codegen::ir::scope_type::type);
                aggregate_ctor.parent_type = codegen_type(ctx);

                return aggregate_ctor;
            });

        _aggregate_ctor->set_eval([this](auto &&, const std::vector<variable *> & args) {
            if (!std::all_of(args.begin(), args.end(), [](auto && arg) { return arg->is_constant(); }))
            {
                return make_ready_future<expression *>(nullptr);
            }

            auto repl = replacements{};
            auto arg_copies = fmap(args, [&](auto && arg) { return arg->clone_with_replacement(repl); });
            return make_ready_future<expression *>(make_variable_expression(make_struct_variable(this->shared_from_this(), std::move(arg_copies))).release());
        });

        _aggregate_ctor->set_name(U"constructor");

        _aggregate_ctor_promise->set(_aggregate_ctor.get());
    }

    void struct_type::_codegen_type(ir_generation_context & ctx) const
    {
        auto actual_type = *_codegen_t;

        // TODO: actual name tracking for this shit

        auto members = fmap(_data_members, [&](auto && member) { return codegen::ir::member{ member->member_codegen_ir(ctx) }; });
        auto type = codegen::ir::variable_type{ _codegen_type_name(ctx), get_scope()->codegen_ir(ctx), 0, members };
        auto aggregate_ctor = _aggregate_ctor->codegen_ir(ctx);
        type.members.push_back(std::move(aggregate_ctor));

        *actual_type = std::move(type);
    }
}
}
