/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/block.h"
#include "vapor/analyzer/function.h"

reaver::vapor::analyzer::_v1::block::block(const reaver::vapor::parser::block & parse, const reaver::vapor::analyzer::_v1::scope & lex_scope) : _parse{ parse }, _block_scope{ lex_scope.copy() }
{
    auto current_scope = std::ref(_block_scope);

    _value = fmap(_parse.block_value, [&](auto && v)
    {
        return fmap(v, make_overload_set(
            [&](const parser::block & v)
            {
                return block(v, current_scope);
            },

            [&](const parser::statement & v)
            {
                return preanalyze_statement(v, current_scope);
            })
        );
    });
}

