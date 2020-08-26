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

#include "vapor/analyzer/expressions/integer.h"
#include "vapor/analyzer/expressions/sized_integer.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/codegen/ir/type.h"
#include "vapor/codegen/ir/variable.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<expression> integer_constant::convert_to(type * target) const
    {
        if (auto sized_target = dynamic_cast<sized_integer *>(target))
        {
            if (_value <= sized_target->max_value() && _value >= sized_target->min_value())
            {
                return std::make_unique<sized_integer_constant>(
                    sized_target, _value, get_scope(), std::nullopt);
            }
        }

        return nullptr;
    }

    constant_init_ir integer_constant::_constinit_ir(ir_generation_context &) const
    {
        return { codegen::ir::integer_value{ _value, 0 } };
    }
}
}
