/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/types/unresolved.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/functional/hash.hpp>

#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/expressions/struct_literal.h"
#include "vapor/analyzer/expressions/typeclass.h"
#include "vapor/analyzer/expressions/unresolved_type.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/overload_set.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/archetype.h"
#include "vapor/analyzer/types/overload_set.h"
#include "vapor/analyzer/types/sized_integer.h"
#include "vapor/analyzer/types/struct.h"
#include "vapor/analyzer/types/typeclass.h"
#include "vapor/analyzer/types/typeclass_instance.h"

#include <google/protobuf/util/message_differencer.h>
#include "expressions/type.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    bool udr_compare::operator()(const proto::user_defined_reference * lhs,
        const proto::user_defined_reference * rhs) const
    {
        return google::protobuf::util::MessageDifferencer::Equals(*lhs, *rhs);
    }

    bool udr_compare::operator()(const synthesized_udr & lhs, const synthesized_udr & rhs) const
    {
        return lhs.module == rhs.module && lhs.name == rhs.name;
    }

    std::size_t udr_hash::operator()(const proto::user_defined_reference * udr) const
    {
        std::size_t seed = 0;

        for (auto && module : udr->module())
        {
            boost::hash_combine(seed, module);
        }

        boost::hash_combine(seed, udr->name());

        return seed;
    }

    std::size_t udr_hash::operator()(const synthesized_udr & udr) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, udr.module);
        boost::hash_combine(seed, udr.name);

        return seed;
    }

    unresolved_type::unresolved_type(user_defined_reference_tag,
        precontext & ctx,
        const proto::user_defined_reference * reference)
        : _reference{ _unresolved_reference{ &ctx,
            std::unique_ptr<synthesized_udr>{
                new synthesized_udr{ boost::algorithm::join(reference->module(), "."),
                    reference->name() } } } }
    {
    }

    unresolved_type::unresolved_type(typeclass_type_tag, std::vector<imported_type> params)
        : _reference{ _unresolved_typeclass_type{ std::move(params) } }
    {
    }

    unresolved_type::unresolved_type(archetype_type_tag, scope * lex_scope, std::u32string name)
        : _reference{ _unresolved_archetype_type{ lex_scope, std::move(name) } }
    {
    }

    unresolved_type::unresolved_type(typeclass_instance_type_tag,
        precontext & ctx,
        const proto::user_defined_reference * tc,
        std::vector<std::unique_ptr<expression>> arguments)
        : _reference{ _unresolved_typeclass_instance_type{ &ctx,
            std::unique_ptr<synthesized_udr>{
                new synthesized_udr{ boost::algorithm::join(tc->module(), "."), tc->name() } },
            std::move(arguments) } }
    {
    }

    future<> unresolved_type::resolve(analysis_context & ctx)
    {
        if (!_analysis_future)
        {
            constexpr auto get_udr = +[](precontext * prectx, const synthesized_udr & ref) {
                auto it = prectx->imported_entities.find(ref);
                if (it == prectx->imported_entities.end())
                {
                    logger::dlog(logger::crash) << "can't find an entity for a user defined reference "
                                                << ref.module << "." << ref.name << styles::def;
                    logger::default_logger().sync();
                    assert(0);
                }
                auto expr = it->second.get();
                assert(expr);
                return expr;
            };

            _analysis_future = std::get<0>(fmap(_reference,
                make_overload_set([&](std::monostate) -> future<> { assert(0); },

                    [&ctx, this](_unresolved_reference & ref) -> future<> {
                        auto expr = get_udr(ref.ctx, *ref.udr);
                        return expr->analyze(ctx).then([expr, this] {
                            auto type_expr = expr->as<type_expression>();
                            assert(type_expr);
                            _resolved = type_expr->get_value();
                        });
                    },

                    [&ctx, this](_unresolved_typeclass_type & tc) -> future<> {
                        return when_all(fmap(tc.parameter_types, [&](auto && imported) {
                            return std::get<0>(fmap(imported,
                                make_overload_set(
                                    [&](type * resolved) { return make_ready_future(resolved); },
                                    [&](std::shared_ptr<unresolved_type> unresolved) {
                                        return unresolved->resolve(ctx).then(
                                            [unresolved] { return unresolved->get_resolved(); });
                                    })));
                        })).then([&](auto && types) { _resolved = ctx.get_typeclass_type(types); });
                    },

                    [this](_unresolved_archetype_type & arch) -> future<> {
                        auto && symb = arch.lex_scope->get(arch.name);
                        auto && type_expr = symb->get_expression()->as<type_expression>();
                        assert(type_expr);
                        assert(dynamic_cast<archetype *>(type_expr->get_value()));
                        _resolved = type_expr->get_value();

                        return make_ready_future();
                    },

                    [&ctx, this](_unresolved_typeclass_instance_type & inst_type) -> future<> {
                        return when_all(
                            fmap(inst_type.arguments, [&](auto && arg) { return arg->analyze(ctx); }))
                            .then([&] { return get_udr(inst_type.ctx, *inst_type.tc); })
                            .then([&](expression * tc_expr) {
                                return tc_expr->analyze(ctx).then([tc_expr] { return tc_expr; });
                            })
                            .then([&](expression * tc_expr) {
                                auto tc = tc_expr->as<typeclass_expression>();
                                return tc->get_typeclass()
                                    ->type_for(ctx,
                                        fmap(inst_type.arguments,
                                            [](auto && arg) {
                                                auto type_expr = arg->template as<type_expression>();
                                                assert(type_expr);
                                                return type_expr->get_value();
                                            }))
                                    .then([this](type * inst_type) { _resolved = inst_type; });
                            });
                    })));
        }

        return _analysis_future.value();
    }

    std::unique_ptr<expression> unresolved_type::get_expression()
    {
        return std::make_unique<unresolved_type_expression>(shared_from_this());
    }

    std::unique_ptr<expression> get_imported_type(precontext & ctx, const proto::type & type)
    {
        switch (type.details_case())
        {
            case proto::type::DetailsCase::kReference:
                return get_imported_type_ref_expr(ctx, type.reference());

            case proto::type::DetailsCase::kStruct:
            {
                auto ret =
                    std::make_unique<struct_literal>(ast_node{}, import_struct_type(ctx, type.struct_()));
                ret->set_name(utf32(ctx.current_symbol));
                return ret;
            }

            case proto::type::DetailsCase::kOverloadSet:
            {
                auto oset = import_overload_set(ctx, type.overload_set());
                auto type = oset->get_type();
                ctx.imported_overload_sets[{ ctx.module_stack.back(), ctx.current_symbol }] = std::move(oset);

                type->set_name(utf32(ctx.current_symbol));
                return make_type_expression(type);
            }

            default:
                assert(0);
        }
    }

    imported_type get_imported_type_ref(precontext & ctx, const proto::type_reference & type)
    {
        switch (type.details_case())
        {
            case proto::type_reference::DetailsCase::kBuiltin:
                switch (type.builtin())
                {
                    case proto::type_:
                        return builtin_types().type.get();
                    case proto::integer:
                        return builtin_types().integer.get();
                    case proto::boolean:
                        return builtin_types().boolean.get();

                    default:
                        assert(0);
                }

            case proto::type_reference::DetailsCase::kSizedInt:
                return ctx.proper.get_sized_integer_type(type.sized_int().size());

            case proto::type_reference::kTypeclass:
            {
                std::vector<proto::type_reference> params{ type.typeclass().parameters().begin(),
                    type.typeclass().parameters().end() };
                return std::make_shared<unresolved_type>(unresolved_typeclass_type,
                    fmap(params, [&](auto && type) { return get_imported_type_ref(ctx, type); }));
            }

            case proto::type_reference::kUserDefined:
            {
                auto & ud = ctx.user_defined_types[&type.user_defined()];
                if (!ud)
                {
                    ud = std::make_shared<unresolved_type>(
                        unresolved_user_defined_reference, ctx, &type.user_defined());
                }

                return ud;
            }

            case proto::type_reference::kTypeclassInstanceType:
            {
                std::vector<std::unique_ptr<expression>> arguments;
                arguments.reserve(type.typeclass_instance_type().arguments_size());
                for (auto && arg : type.typeclass_instance_type().arguments())
                {
                    arguments.push_back(get_imported_type_ref_expr(ctx, arg));
                }

                return std::make_shared<unresolved_type>(unresolved_typeclass_instance_type,
                    ctx,
                    &type.typeclass_instance_type().typeclass(),
                    std::move(arguments));
            }

            case proto::type_reference::kArchetype:
                return std::make_shared<unresolved_type>(
                    unresolved_archetype_type, ctx.current_lex_scope, utf32(type.archetype().name()));

            default:
                assert(0);
        }
    }

    std::unique_ptr<expression> get_imported_type_ref_expr(precontext & ctx,
        const proto::type_reference & reference)
    {
        switch (reference.details_case())
        {
            case proto::type_reference::DetailsCase::kBuiltin:
            case proto::type_reference::DetailsCase::kSizedInt:
                return make_expression_ref(
                    std::get<0>(get_imported_type_ref(ctx, reference))->get_expression(), std::nullopt);

            case proto::type_reference::DetailsCase::kUserDefined:
            case proto::type_reference::DetailsCase::kTypeclass:
            case proto::type_reference::DetailsCase::kTypeclassInstanceType:
            case proto::type_reference::kArchetype:
                return std::get<1>(get_imported_type_ref(ctx, reference))->get_expression();

            default:
                assert(0);
        }
    }
}
}
