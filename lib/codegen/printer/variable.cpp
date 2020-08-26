/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017, 2019 Michał "Griwes" Dominiak
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

#include "vapor/codegen/ir/type.h"
#include "vapor/codegen/printer.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    std::u32string ir_printer::generate_definition(const ir::variable & var, codegen_context & ctx)
    {
        if (var.type == ir::builtin_types().type)
        {
            assert(var.initializer);
            auto refers_to = std::get_if<std::shared_ptr<ir::type>>(var.initializer.value().operator->());
            assert(refers_to);
            ctx.put_into_global_before += ctx.define_if_necessary(*refers_to);
            return {};
        }

        ctx.put_into_global += ctx.define_if_necessary(var.type);
        return U"define variable @ " + _pointer_to_string(&var) + U" : type @ "
            + _pointer_to_string(var.type.get()) + U" `" + (var.name ? var.name.value() : U"") + U"`\n";
    }

    std::u32string ir_printer::generate_definition(const ir::member_variable & mem_var, codegen_context & ctx)
    {
        return U"define member variable @ " + _pointer_to_string(&mem_var) + U" : type @ "
            + _pointer_to_string(mem_var.type.get()) + U" `" + mem_var.name + U"`\n";
    }
}
}
