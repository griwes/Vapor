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
#include <string>
#include <unordered_map>

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class symbol;

            class scope
            {
            public:
                scope() = default;

                scope(const scope &) = default;
                scope(scope &&) = default;

                void add_symbol(const std::u32string & name, symbol & symb)
                {
                    if (_symbols.find(name) != _symbols.end())
                    {
                        // TODO: throw an error
                        assert(0);
                    }

                    _symbols.emplace(name, symb);
                }

                auto copy() const
                {
                    return scope{ *this, _key{} };
                }

            protected:
                struct _key {};

                scope(const scope & parent, _key) : _parent(parent)
                {
                }

                optional<const scope &> _parent;
                std::unordered_map<std::u32string, symbol &> _symbols;
            };
        }}
    }
}

