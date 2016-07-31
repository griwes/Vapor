/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014 Michał "Griwes" Dominiak
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

#include <memory>

#include <reaver/optional.h>

#include "vapor/parser/declaration.h"
#include "vapor/analyzer/symbol.h"
#include "vapor/analyzer/expression.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class declaration : public std::enable_shared_from_this<declaration>
            {
            public:
                declaration(scope & lex_scope, const parser::declaration & parse)
                    : _name{ parse.identifier.string },
                    _declared_symbol{ symbol(_name, lex_scope, variable()) },
                    _initializer_expression{ preanalyze_expression(parse.rhs, lex_scope) },
                    _type_specifier{ fmap(parse.type_expression, [&](auto v){ return preanalyze_expression(v, lex_scope); })  },
                    _parse{ parse }
                {
                    lex_scope.add_symbol(_name, _declared_symbol);
                }

                const auto & name() const
                {
                    return _name;
                }

                const auto & parse() const
                {
                    return _parse;
                }

                auto declared_symbol() const
                {
                    return _declared_symbol;
                }

                auto initializer_expression() const
                {
                    return _initializer_expression;
                }

                void analyze()
                {
                    _declared_symbol.variable()._declaration = *this;

                    fmap(_type_specifier, [](auto && v) { analyzer::analyze(v); return unit{}; });
                    analyzer::analyze(_initializer_expression);
                    _declared_symbol.variable()._type = _type_specifier ? value_of(*_type_specifier) : type_of(_initializer_expression);
                }

            private:
                std::u32string _name;
                symbol _declared_symbol;
                expression _initializer_expression;
                optional<expression> _type_specifier;
                const parser::declaration & _parse;
            };
        }}
    }
}
