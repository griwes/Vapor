/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include <reaver/overloads.h>
#include <reaver/variant.h>

#include "vapor/codegen/ir/entity.h"
#include "vapor/codegen/ir/variable.h"
#include "vapor/codegen/printer.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    std::u32string ir_printer::generate_definitions(std::vector<ir::entity> & module, codegen_context & ctx)
    {
        std::u32string ret;

        for (auto && symbol : module)
        {
            ret += std::get<0>(fmap(symbol,
                make_overload_set(
                    [&](std::shared_ptr<ir::variable> & var) { return this->generate_definition(*var, ctx); },
                    [&](ir::function & fn) { return this->generate_definition(fn, ctx); })));
        }

        return ret;
    }

    std::u32string ir_printer::_to_string(const ir::value & val)
    {
        return std::get<std::u32string>(fmap(val,
            make_overload_set(
                [&](const ir::integer_value & val) {
                    std::stringstream ss;
                    ss << val.value;

                    if (val.size)
                    {
                        ss << "_" << val.size.value();
                    }

                    return utf32(ss.str());
                },
                [&](const ir::boolean_value & val) -> std::u32string {
                    return val.value ? U"true" : U"false";
                },
                [&](const std::shared_ptr<ir::variable> & var) {
                    return U"variable @ " + _pointer_to_string(var.get()) + U" `"
                        + (var->name ? var->name.value() : U"") + U"`";
                },
                [&](const ir::label & label) { return label.name; },
                [&](const ir::struct_value & struct_val) -> std::u32string {
                    return U"type @ " + _pointer_to_string(struct_val.type.get()) + U"{ "
                        + boost::algorithm::join(
                            fmap(struct_val.fields, [&](auto && v) { return _to_string(v); }), U", ")
                        + U" }";
                },
                [&](const ir::function_value & fn) { return U"function pointer to " + fn.name; },
                [&](auto &&) {
                    assert(0);
                    return unit{};
                })));
    }
}
}
