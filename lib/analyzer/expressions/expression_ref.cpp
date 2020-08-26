/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2019 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/semantic/symbol.h"

#include "expressions/type.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<google::protobuf::Message> expression_ref::_generate_interface() const
    {
        if (get_type() == builtin_types().type)
        {
            auto ret = std::make_unique<proto::type>();
            ret->set_allocated_reference(
                _referenced->as<type_expression>()->get_value()->generate_interface_reference().release());
            return ret;
        }

        assert(0);
    }
}
}
