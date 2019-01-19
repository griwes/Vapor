/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2017, 2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/expression_list.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/block.h"
#include "vapor/analyzer/statements/return.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    future<> block::_analyze(analysis_context & ctx)
    {
        auto fut = when_all(fmap(_statements, [&](auto && stmt) { return stmt->analyze(ctx); }));

        fmap(_value_expr, [&](auto && expr) {
            fut = fut.then([&ctx, expr = expr.get()] { return expr->analyze(ctx); });
            return unit{};
        });

        return fut;
    }
}
}
