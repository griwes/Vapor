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

#include "vapor/analyzer/statements/declaration.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    future<> declaration::_analyze(analysis_context & ctx)
    {
        auto fut = make_ready_future();

        fmap(_type_specifier, [&](auto && expr) {
            fut = fut.then([&]() { return expr->analyze(ctx); }).then([&]() {
                auto && type_expr = _type_specifier.get();
                auto && type_var = type_expr->get_variable();
                assert(type_var->get_type() == builtin_types().type.get());
                assert(type_var->is_constant());
            });

            return unit{};
        });

        fmap(_init_expr, [&](auto && expr) {
            fut = fut.then([&]() { return expr->analyze(ctx); }).then([&] { _declared_symbol->set_variable(_init_expr.get()->get_variable()); });

            fmap(_type_specifier, [&](auto && expr) {
                fut = fut.then([&]() {
                    auto && type_var = static_cast<type_variable *>(expr->get_variable());
                    assert(_init_expr.get()->get_type() == type_var->get_value());
                });

                return unit{};
            });

            return unit{};
        });

        return fut;
    }
}
}
