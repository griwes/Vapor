/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2018 Michał "Griwes" Dominiak
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
#include "vapor/analyzer/expressions/member_access.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/struct.h"
#include "vapor/analyzer/expressions/struct_value.h"
#include "vapor/analyzer/statements/declaration.h"
#include "vapor/analyzer/symbol.h"
#include "vapor/parser/expr.h"

#include "vapor/analyzer/expressions/integer.h"

#include "types/struct.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<struct_type> make_struct_type(precontext & ctx, const parser::struct_literal & parse, scope * lex_scope)
    {
        auto member_scope = lex_scope->clone_for_class();

        std::vector<std::unique_ptr<declaration>> decls;

        fmap(parse.members, [&](auto && member) {
            fmap(member,
                make_overload_set(
                    [&](const parser::declaration & decl) {
                        auto scope = member_scope.get();
                        auto decl_stmt = preanalyze_member_declaration(ctx, decl, scope);
                        assert(scope == member_scope.get());

                        decls.push_back(std::move(decl_stmt));

                        return unit{};
                    },

                    [&](const parser::function_definition & func) {
                        assert(0);
                        return unit{};
                    }));

            return unit{};
        });

        member_scope->close();

        return std::make_unique<struct_type>(make_node(parse), std::move(member_scope), std::move(decls));
    }

    struct_type::~struct_type() = default;

    struct_type::struct_type(ast_node parse, std::unique_ptr<scope> member_scope, std::vector<std::unique_ptr<declaration>> member_decls)
        : user_defined_type{ std::move(member_scope) }, _parse{ parse }, _data_members_declarations{ std::move(member_decls) }
    {
        auto ctor_pair = make_promise<function *>();
        _aggregate_ctor_future = std::move(ctor_pair.future);
        _aggregate_ctor_promise = std::move(ctor_pair.promise);

        auto copy_pair = make_promise<function *>();
        _aggregate_copy_ctor_future = std::move(copy_pair.future);
        _aggregate_copy_ctor_promise = std::move(copy_pair.promise);
    }

    void struct_type::generate_constructors()
    {
        _data_members = fmap(_data_members_declarations, [&](auto && member) {
            auto ret = member->declared_member();
            ret->set_parent_type(this);
            return ret;
        });

        _aggregate_ctor = make_function("struct type constructor",
            get_expression(),
            fmap(_data_members, [](auto && member) -> expression * { return member; }),
            [&](auto && ctx) -> codegen::ir::function {
                auto ir_type = this->codegen_type(ctx);
                auto args = fmap(_data_members, [&](auto && member) {
                    auto ret = codegen::ir::make_variable(member->get_type()->codegen_type(ctx));
                    return ret;
                });

                auto result = codegen::ir::make_variable(ir_type);

                auto scopes = this->get_scope()->codegen_ir();
                scopes.emplace_back(get_name(), codegen::ir::scope_type::type);

                return { U"constructor",
                    std::move(scopes),
                    args,
                    result,
                    { codegen::ir::instruction{ std::nullopt,
                          std::nullopt,
                          { boost::typeindex::type_id<codegen::ir::aggregate_init_instruction>() },
                          fmap(args, [](auto && arg) -> codegen::ir::value { return arg; }),
                          result },
                        codegen::ir::instruction{
                            std::nullopt, std::nullopt, { boost::typeindex::type_id<codegen::ir::return_instruction>() }, { result }, result } } };
            });

        _aggregate_ctor->set_scopes_generator([this](auto && ctx) { return this->codegen_scopes(ctx); });

        _aggregate_ctor->set_eval([this](auto &&, const std::vector<expression *> & args) {
            if (!std::equal(args.begin(), args.end(), _data_members.begin(), [](auto && arg, auto && member) { return arg->get_type() == member->get_type(); }))
            {
                assert(0);
            }

            if (!std::all_of(args.begin(), args.end(), [](auto && arg) { return arg->is_constant(); }))
            {
                return make_ready_future<expression *>(nullptr);
            }

            auto repl = replacements{};
            auto arg_copies = fmap(args, [&](auto && arg) { return repl.claim(arg); });
            return make_ready_future<expression *>(make_struct_expression(this->shared_from_this(), std::move(arg_copies)).release());
        });

        _aggregate_ctor->set_name(U"constructor");

        _aggregate_ctor_promise->set(_aggregate_ctor.get());

        auto data_members = fmap(_data_members, [&](auto && member) -> expression * {
            auto param = make_member_expression(this, member->get_name(), member->get_type());
            auto def_value = make_member_access_expression(member->get_name(), member->get_type());

            param->set_default_value(def_value.get());

            auto param_ptr = param.get();
            _member_copy_arguments.push_back(std::move(param));
            _member_copy_arguments.push_back(std::move(def_value));
            return param_ptr;
        });

        _this_argument = make_runtime_value(this);
        data_members.insert(data_members.begin(), _this_argument.get());

        _aggregate_copy_ctor =
            make_function("struct type copy replacement constructor", get_expression(), data_members, [&](auto && ctx) -> codegen::ir::function {
                auto ir_type = this->codegen_type(ctx);
                auto args = fmap(_data_members, [&](auto && member) {
                    auto ret = codegen::ir::make_variable(member->get_type()->codegen_type(ctx));
                    return ret;
                });
                auto result = codegen::ir::make_variable(ir_type);

                auto scopes = this->get_scope()->codegen_ir();
                scopes.emplace_back(get_name(), codegen::ir::scope_type::type);

                return { U"replacing_copy_constructor",
                    std::move(scopes),
                    args,
                    result,
                    { codegen::ir::instruction{ std::nullopt,
                          std::nullopt,
                          { boost::typeindex::type_id<codegen::ir::aggregate_init_instruction>() },
                          fmap(args, [](auto && arg) -> codegen::ir::value { return arg; }),
                          result },
                        codegen::ir::instruction{
                            std::nullopt, std::nullopt, { boost::typeindex::type_id<codegen::ir::return_instruction>() }, { result }, result } } };
            });

        _aggregate_copy_ctor->set_scopes_generator([this](auto && ctx) { return this->codegen_scopes(ctx); });

        _aggregate_copy_ctor->set_eval([this](auto &&, std::vector<expression *> args) {
            auto base = args.front();
            args.erase(args.begin());

            if (base->get_type() != this || !std::equal(args.begin(), args.end(), _data_members.begin(), [](auto && arg, auto && member) {
                    return arg->get_type() == member->get_type();
                }))
            {
                logger::default_logger().sync();
                assert(0);
            }

            for (auto && arg : args)
            {
                if (auto member_arg = arg->as<member_access_expression>())
                {
                    auto actual_arg = base->get_member(member_arg->get_name());
                    arg = actual_arg ? actual_arg : arg;
                }
            }

            if (!std::all_of(args.begin(), args.end(), [](auto && arg) { return arg->is_constant(); }))
            {
                return make_ready_future<expression *>(nullptr);
            }

            auto repl = replacements{};
            for (std::size_t i = 0; i < _data_members.size(); ++i)
            {
                repl.add_replacement(base->get_member(_data_members[i]->get_name()), args[i]);
            }
            return make_ready_future(repl.claim(base->_get_replacement()).release());
        });

        _aggregate_copy_ctor->set_name(U"replacing_copy_constructor");
        _aggregate_copy_ctor->make_member();

        _aggregate_copy_ctor_promise->set(_aggregate_copy_ctor.get());
    }

    void struct_type::_codegen_type(ir_generation_context & ctx) const
    {
        auto actual_type = *_codegen_t;

        auto type = codegen::ir::variable_type{ get_name(), get_scope()->codegen_ir(), 0, {} };

        auto members = fmap(_data_members, [&](auto && member) { return codegen::ir::member{ member->member_codegen_ir(ctx) }; });

        auto scopes = this->get_scope()->codegen_ir();
        scopes.emplace_back(type.name, codegen::ir::scope_type::type);

        auto add_fn = [&](auto && fn) {
            auto fn_ir = fn->codegen_ir(ctx);
            fn_ir.scopes = scopes;
            fn_ir.parent_type = actual_type;
            members.push_back(codegen::ir::member{ fn_ir });
            ctx.add_generated_function(fn.get());
        };
        add_fn(_aggregate_ctor);
        add_fn(_aggregate_copy_ctor);

        type.members = std::move(members);

        *actual_type = std::move(type);
    }

    void struct_type::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << "struct type";
        print_address_range(os, this);
        os << '\n';
    }

    std::unique_ptr<google::protobuf::Message> struct_type::_user_defined_interface() const
    {
        auto t = std::make_unique<proto::struct_type>();

        for (auto && member : _data_members)
        {
            auto proto_member = t->add_data_members();

            proto_member->set_name(utf8(member->get_name()));
            proto_member->set_allocated_type(member->get_type()->generate_interface_reference().release());
        }

        return std::move(t);
    }
}
}
