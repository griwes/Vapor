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

#include "vapor/analyzer/semantic/overloads.h"

#include <reaver/id.h>

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/conversion.h"
#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/member.h"
#include "vapor/analyzer/expressions/member_access.h"
#include "vapor/analyzer/expressions/member_assignment.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    enum class overload_match
    {
        first_better,
        equal,
        second_better
    };

    overload_match compare_overloads(function * lhs, function * rhs)
    {
        assert(0);
        assert(lhs);
        assert(rhs);
        return overload_match::equal;
    }

    bool is_valid(function * overload,
        const std::vector<expression *> & arguments,
        expression * base = nullptr)
    {
        auto param_begin = overload->parameters().begin();
        auto param_end = overload->parameters().end();
        auto arg_begin = arguments.begin();
        auto arg_end = arguments.end();

        if (overload->is_member())
        {
            if (!base)
            {
                logger::dlog() << overload->explain()
                               << " not considered; is a member function, but the base expression is null";
                return false;
            }

            // TODO: conversions? possibly "interface inheritance" that operator. by Bjarne attempts (badly)?
            if ((*param_begin)->get_type() != base->get_type())
            {
                logger::dlog()
                    << overload->explain()
                    << " not considered; is a member function, but the base expression is of the wrong type";
                logger::dlog() << "expected expression type: " << (*param_begin)->get_type()->explain();
                logger::dlog() << "actual expression type: " << base->get_type()->explain();
                return false;
            }

            assert(param_begin != param_end);
            ++param_begin;
        }

        std::vector<expression *> provided_params;

        auto it = arg_begin;
        if ((it = std::find_if(it, arg_end, [](auto && arg) { return arg->is_member_assignment(); }))
            != arg_end)
        {
            auto arg_begin = it;
            auto possible_arg_end = it;

            if ((it = std::find_if(it, arg_end, [](auto && arg) { return !arg->is_member_assignment(); }))
                != arg_end)
            {
                assert(!"a non-mem-assignment argument after a mem-assignment argument");
            }

            // this is kind of a hack to allow passing member assignments that are *before the first matching
            // one* to pack arguments
            //
            // this will need a rework
            //
            // basically this is necessary to get the generic constructor to work
            // and I'm wondering wether this magic shouldn't just be limited to that single thing
            //
            // but I guess it'll be generally allowed to allow making forwarding wrappers that accept member
            // assignments
            bool succeeded_before = false;

            while (arg_begin != arguments.end())
            {
                auto arg = (*arg_begin)->as<member_assignment_expression>();
                assert(arg);

                auto it = param_begin;
                if ((it = std::find_if(param_begin,
                         param_end,
                         [&](expression * param) {
                             auto member_param = param->as<member_expression>();
                             if (!member_param)
                             {
                                 return false;
                             }
                             return member_param->get_name() == arg->member_name();
                         }))
                    != param_end)
                {
                    assert((*it)->get_type()->matches(arg->get_assigned_type()));
                    succeeded_before = true;

                    provided_params.push_back(*it);

                    ++arg_begin;
                    continue;
                }

                if (succeeded_before)
                {
                    logger::dlog() << overload->explain()
                                   << " not considered; mismatch in member assignment arguments; ."
                                   << utf8(arg->member_name()) << " did not match any members";
                    return false;
                }

                ++arg_begin;
            }

            if (succeeded_before)
            {
                arg_end = possible_arg_end;
            }
        }

        std::vector<type *> matching_space;
        matching_space.reserve(arg_end - arg_begin);

        // TODO: this will need some help in the future
        // in particular to be able to handle cases like
        // with (Ts... : type)
        // function foo(ts : Ts.uref..., last : int);
        // I know this can be done, but is more complex and we don't need it right now
        // and we can probably live with it not working until there's actually syntax for the above in the
        // language (i.e. when I'll be adding packs to the language for more than just the generic ctor)
        while (arg_begin != arg_end && param_begin != param_end)
        {
            if (std::find(provided_params.begin(), provided_params.end(), *param_begin)
                != provided_params.end())
            {
                // should this be a hard error? probably not
                assert(0);
                return false;
            }

            matching_space.clear();

            auto && param_type = (*param_begin)->get_type();

            if (!param_type->matches((*arg_begin)->get_type()))
            {
                if (!param_type->matches(matching_space))
                {
                    logger::dlog() << overload->explain() << " not considered; argument #"
                                   << arg_begin - arguments.begin() << " does not match the parameter #"
                                   << param_begin - overload->parameters().begin();
                    logger::dlog() << "argument type: " << (*arg_begin)->get_type()->explain();
                    logger::dlog() << "parameter type: " << (*param_begin)->get_type()->explain();
                    return false;
                }

                ++param_begin;
                continue;
            }

            bool last_matched;

            do
            {
                matching_space.push_back((*arg_begin)->get_type());
            } while ((last_matched = param_type->matches(matching_space)) && ++arg_begin != arg_end);

            if (matching_space.size() == 1 && !last_matched)
            {
                ++arg_begin;
            }

            ++param_begin;
        }

        if (arg_begin != arg_end)
        {
            logger::dlog() << overload->explain() << " not considered; argument #"
                           << arg_begin - arguments.begin() << " exceeds the number of parameters";
            logger::dlog() << "argument type: " << (*arg_begin)->get_type()->explain();
            return false;
        }

        auto ret = std::all_of(param_begin, param_end, [&](auto && param) {
            return std::find(provided_params.begin(), provided_params.end(), param) != provided_params.end()
                || param->get_default_value();
        });

        if (!ret)
        {
            logger::dlog() << overload->explain()
                           << " not considered; some not provided parameters do not have a default value";
        }

        return ret;
    }

    auto prepare_actual_arguments(scope * lex_scope,
        const std::optional<std::u32string> & name,
        function * overload,
        std::vector<expression *> arguments,
        expression * base = nullptr)
    {
        std::vector<expression *> ret;

        std::vector<expression *> matched;

        std::vector<type *> matching_space;
        std::vector<expression *> used_args;

        auto arg_begin = arguments.begin();
        auto arg_end = arguments.end();
        auto param_begin = overload->parameters().begin();
        auto param_end = overload->parameters().end();

        if (overload->is_member())
        {
            assert(base);
            ret.push_back(base);
            ++param_begin;
        }

        auto handle_conversion = [&](expression *& expr, type * conv) {
            // stupid leak
            expr = make_conversion_expression(expr, conv, lex_scope, name).release();
        };

        // I actually do need to erase my ownerships here
        // make the typeclasses thingy actually usable already, dammit

        while (arg_begin != arg_end)
        {
            assert(param_begin != param_end);

            matching_space.clear();
            matched.clear();

            auto && param_type = (*param_begin)->get_type();

            if (!param_type->matches((*arg_begin)->get_type()))
            {
                if (!param_type->matches(matching_space))
                {
                    if ((*arg_begin)->is_member_assignment())
                    {
                        break;
                    }

                    assert(0);
                }

                ++param_begin;
                continue;
            }

            bool last_matched;

            do
            {
                matching_space.push_back((*arg_begin)->get_type());
                matched.push_back(*arg_begin);
            } while ((last_matched = param_type->matches(matching_space)) && ++arg_begin != arg_end);

            if (matching_space.size() == 1 && !last_matched)
            {
                if (param_type->needs_conversion(matched.back()->get_type()))
                {
                    handle_conversion(matched.back(), param_type);
                }

                ++arg_begin;
            }

            std::copy(matched.begin(), matched.end(), std::back_inserter(ret));

            ++param_begin;
        }

        while (param_begin != param_end)
        {
            auto param = *param_begin;

            if (auto member_param = param->as<member_expression>())
            {
                auto it = arg_begin;

                while (it != arg_end)
                {
                    auto assignment = (*it)->as<member_assignment_expression>();
                    if (!assignment || assignment->member_name() != member_param->get_name())
                    {
                        ++it;
                        continue;
                    }

                    ret.push_back(assignment->get_rhs());
                    assert(!ret.back()->is_member_assignment());

                    if (member_param->get_type()->needs_conversion(ret.back()->get_type()))
                    {
                        handle_conversion(ret.back(), member_param->get_type());
                    }

                    ++param_begin;
                    break;
                }

                if (it != arg_end)
                {
                    continue;
                }
            }

            assert(param->get_default_value());
            ret.push_back(param->get_default_value());
            assert(!ret.back()->is_member_assignment());
            ++param_begin;
        }

        return ret;
    }

    future<std::unique_ptr<expression>> select_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        std::vector<expression *> arguments,
        std::vector<function *> possible_overloads,
        expression * base)
    {
        auto original = possible_overloads;

        possible_overloads.erase(std::remove_if(possible_overloads.begin(),
                                     possible_overloads.end(),
                                     [&](auto && overload) { return !is_valid(overload, arguments, base); }),
            possible_overloads.end());

        std::sort(possible_overloads.begin(), possible_overloads.end(), [](auto && lhs, auto && rhs) {
            return compare_overloads(lhs, rhs) == overload_match::first_better;
        });

        // we might have ambiguous overloads
        // so we might have multiple "best matches"
        std::vector<function *> best_matches;
        for (auto && overload : possible_overloads)
        {
            if (!best_matches.empty()
                && compare_overloads(overload, best_matches.back()) == overload_match::equal)
            {
                break;
            }

            best_matches.push_back(overload);
        }

        if (best_matches.empty())
        {
            logger::dlog(logger::error) << "no matching overload found for expression at " << range;
            logger::dlog(logger::info) << "originally possible overloads:";
            fmap(original, [](auto && overload) {
                logger::dlog() << overload->explain();
                return unit{};
            });
            logger::default_logger().sync();
            std::terminate();
        }

        else if (best_matches.size() != 1)
        {
            logger::dlog(logger::error) << "multiple matching overloads found for expression at " << range;
            logger::dlog(logger::info) << "possible overloads:";
            fmap(best_matches, [](auto && overload) {
                logger::dlog() << overload->explain();
                return unit{};
            });
            logger::default_logger().sync();
            std::terminate();
        }

        auto overload = best_matches.front();
        auto actual_arguments = prepare_actual_arguments(lex_scope, name, overload, arguments, base);
        auto ret =
            make_call_expression(overload, base, std::move(actual_arguments), lex_scope, std::move(name));
        return make_ready_future<std::unique_ptr<expression>>(std::move(ret));
    }

    future<std::unique_ptr<expression>> resolve_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        expression * lhs,
        expression * rhs,
        lexer::token_type op)
    {
        return lhs->get_type()->get_candidates(op).then(
            [&ctx, range, lex_scope, name = std::move(name), lhs, rhs](auto && overloads) {
                return select_overload(
                    ctx, range, lex_scope, std::move(name), std::vector<expression *>{ lhs, rhs }, overloads);
            });
    }

    future<std::unique_ptr<expression>> resolve_overload(analysis_context & ctx,
        const range_type & range,
        scope * lex_scope,
        std::optional<std::u32string> name,
        expression * base_expr,
        lexer::token_type bracket_type,
        std::vector<expression *> arguments)
    {
        // mutable for `arguments`
        return base_expr->get_type()
            ->get_candidates(bracket_type)
            .then([&ctx](auto overloads) {
                return when_all(fmap(overloads, [&ctx](auto && overload) {
                    return when_all(fmap(
                        overload->parameters(), [&ctx](auto && overload) { return overload->analyze(ctx); }));
                })).then([overloads] { return overloads; });
            })
            .then([&ctx, range, lex_scope, name = std::move(name), arguments, base_expr](
                      auto overloads) mutable {
                return select_overload(
                    ctx, range, lex_scope, std::move(name), std::move(arguments), overloads, base_expr);
            });
    }
}
}
