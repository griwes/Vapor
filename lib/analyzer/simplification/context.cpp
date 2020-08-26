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

#include "vapor/analyzer/simplification/context.h"

#include <boost/functional/hash.hpp>

#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/symbol.h"

std::size_t std::hash<reaver::vapor::analyzer::call_frame>::operator()(
    const reaver::vapor::analyzer::call_frame & frame) const
{
    std::size_t seed = 0;

    boost::hash_combine(seed, frame.function);
    boost::hash_combine(seed, frame.arguments.size());
    for (auto && arg : frame.arguments)
    {
        boost::hash_combine(seed, *arg);
    }

    return seed;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    bool operator==(const call_frame & lhs, const call_frame & rhs)
    {
        return lhs.function == rhs.function
            && std::mismatch(lhs.arguments.begin() + lhs.function->is_member(),
                   lhs.arguments.end(),
                   rhs.arguments.begin() + lhs.function->is_member(),
                   rhs.arguments.end(),
                   [](auto && lhs, auto && rhs) { return !lhs->is_different_constant(rhs); })
                   .first
            == lhs.arguments.end();
    }

    void cached_results::save_call_result(call_frame frame, std::unique_ptr<expression> expr)
    {
        auto it = _cached_call_results.find(frame);
        if (it == _cached_call_results.end())
        {
            replacements repl;
            auto owning =
                fmap(frame.arguments, [&](auto && arg) { return repl.claim(arg->_get_replacement()); });
            auto raw = fmap(owning, [](auto && arg) { return arg.get(); });
            std::move(owning.begin(), owning.end(), std::back_inserter(_key_store));
            _cached_call_results.emplace(call_frame{ frame.function, std::move(raw) }, std::move(expr));
        }
    }

    std::unique_ptr<expression> cached_results::get_call_result(call_frame frame) const
    {
        auto it = _cached_call_results.find(frame);
        if (it != _cached_call_results.end())
        {
            replacements repl;
            return repl.claim(it->second.get());
        }

        return {};
    }

    simplification_context::~simplification_context()
    {
        for (auto && kept_alive : _keep_alive_stmt)
        {
            auto kept_raw = kept_alive.get();
            logger::dlog(logger::trace) << "[~" << this << "] Destroying keep-alive " << kept_raw << " ("
                                        << typeid(*kept_raw).name() << ")";
        }
    }

    void simplification_context::_handle_expressions(expression * ptr, future<expression *> & fut)
    {
        if (_statement_futures.find(ptr) != _statement_futures.end())
        {
            return;
        }

        _statement_futures.emplace(
            ptr, fut.then([](auto && expr) { return static_cast<statement *>(expr); }));
    }

    void simplification_context::keep_alive(statement * ptr)
    {
        auto inserted = _keep_alive_stmt.emplace(ptr).second;
        assert(inserted);
    }
}
}
