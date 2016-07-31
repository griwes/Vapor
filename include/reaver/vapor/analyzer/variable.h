/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2015 Michał "Griwes" Dominiak
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

#include "vapor/range.h"
#include "vapor/analyzer/value.h"

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            class value;
            class declaration;

            class variable
            {
            public:
                friend class declaration;

                variable() : _type{ none }, _value{ none }
                {
                }

                explicit variable(optional<const variable &> type) : _type{ type }, _value{ none }
                {
                }

                variable(optional<const variable &> type, optional<value> value) : _type{ type }, _value{ value }
                {
                }

                variable(const variable &) = default;
                variable(variable &&) noexcept = default;

                void analyze()
                {
                    assert(0);
                }

                vapor::range_type range;

            private:
                optional<const variable &> _type;
                optional<value> _value;
                optional<declaration &> _declaration;
            };
        }}
    }
}

