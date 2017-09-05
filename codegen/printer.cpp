/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017 Michał "Griwes" Dominiak
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

#include "vapor/codegen/printer.h"
#include "vapor/codegen/ir/module.h"
#include "vapor/codegen/ir/variable.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    std::u32string ir_printer::generate_definitions(ir::module & module, codegen_context & ctx)
    {
        std::u32string ret;

        for (auto && symbol : module.symbols)
        {
            ret += get<0>(fmap(symbol,
                make_overload_set([&](std::shared_ptr<ir::variable> & var) { return this->generate_definition(*var, ctx); },
                    [&](ir::function & fn) { return this->generate_definition(fn, ctx); })));
        }

        return ret;
    }

    std::u32string ir_printer::_to_string(const ir::value & val)
    {
        return get<std::u32string>(fmap(val,
            make_overload_set(
                [&](const ir::integer_value & val) {
                    std::stringstream ss;
                    ss << val.value;

                    if (val.size)
                    {
                        ss << "_" << val.size.get();
                    }

                    return utf32(ss.str());
                },
                [&](const ir::boolean_value & val) -> std::u32string { return val.value ? U"true" : U"false"; },
                [&](const std::shared_ptr<ir::variable> & var) {
                    return U"variable @ " + _pointer_to_string(var.get()) + U" `" + (var->name ? _scope_string(var->scopes) + U"." + var->name.get() : U"")
                        + U"`";
                },
                [&](const ir::label & label) { return (label.scopes.empty() ? U"" : _scope_string(label.scopes) + U".") + label.name; },
                [&](const ir::struct_value & struct_val) -> std::u32string {
                    return U"type @ " + _pointer_to_string(struct_val.type.get()) + U"{ "
                        + boost::algorithm::join(fmap(struct_val.fields, [&](auto && v) { return _to_string(v); }), U", ") + U" }";
                },
                [&](auto &&) {
                    assert(0);
                    return unit{};
                })));
    }
}
}
