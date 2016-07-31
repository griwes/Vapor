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

#pragma once

#include <vector>
#include <string>
#include <map>

#include <boost/algorithm/string.hpp>

#include <reaver/prelude/functor.h>

#include "vapor/range.h"
#include "vapor/parser/module.h"
#include "vapor/parser/id_expression.h"
#include "vapor/analyzer/declaration.h"
#include "vapor/analyzer/scope.h"
#include "vapor/analyzer/import.h"
#include "vapor/analyzer/symbol.h"
#include "vapor/analyzer/helpers.h"
#include "vapor/analyzer/statement.h"
#include "vapor/analyzer/function.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class module
            {
            public:
                module(const parser::module & parse) : _parse{ parse }, _statements{ fmap(parse.statements, [&](const auto & statement)
                    { return preanalyze_statement(statement, _module_scope); }) }
                {
                }

                void analyze()
                {
                    fmap(_statements, [&](auto & statement) { return fmap(statement, make_overload_set(
                        [&](declaration & decl)
                        {
                            decl.analyze();
                            return unit{};
                        },

                        [&](expression &)
                        {
                            throw exception{ logger::crash } << "got invalid statement in module; fix the parser";
                            return unit{};
                        },

                        [](auto &&...)
                        {
                            assert(0);
                            return unit{};
                        }
                    )); });
                }

                std::u32string name() const
                {
                    return boost::join(fmap(_parse.name.id_expression_value, [](auto && elem) -> decltype(auto) { return elem.string; }), ".");
                }

            private:
                scope _module_scope;
                const parser::module & _parse;
                std::vector<statement> _statements;
            };
        }}
    }
}

