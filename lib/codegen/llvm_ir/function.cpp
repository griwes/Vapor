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

#include "vapor/codegen/ir/function.h"
#include "vapor/codegen/llvm_ir.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    std::u32string llvm_ir_generator::generate_definition(ir::function & fn, codegen_context & ctx)
    {
        std::u32string ret;

        if (auto type = fn.parent_type.lock())
        {
            ctx.put_into_global_before += ctx.define_if_necessary(type);
        }

        ctx.put_into_global_before += ctx.define_if_necessary(ir::get_type(fn.return_value));
        for (auto && param : fn.parameters)
        {
            ctx.put_into_global_before += ctx.define_if_necessary(ir::get_type(param));
        }

        auto old = ctx.in_function_definition;
        ctx.in_function_definition = true;

        ret += fn.is_defined ? U"define " : U"declare ";
        ret += !fn.is_defined || fn.is_exported ? U"" : U"internal ";
        ret += type_name(ir::get_type(fn.return_value), ctx);
        ret += U" @\"" + function_name(fn, ctx);
        ret += U"\"(\n";
        for (auto && param : fn.parameters)
        {
            ret += U"    " + type_name(ir::get_type(param), ctx) + U" " + variable_name(*param, ctx) + U",\n";
        }

        if (!fn.parameters.empty())
        {
            ret.pop_back();
            ret.pop_back();
            ret.push_back(U'\n');
        }
        ret += U")\n";

        if (fn.is_defined)
        {
            ret += U"{\n";

            // there needs to be an entry label
            ret += U"entry:\n";

            for (auto && inst : fn.instructions)
            {
                ret += generate(inst, ctx);
            }

            ret += U"}\n\n";
        }

        else
        {
            ret += U"\n";
        }

        ctx.in_function_definition = old;

        // quick and dirty, but this should work!
        if (fn.is_entry)
        {
            ret += U"define i32 @__entry_call_thunk(i32 %arg)\n";
            ret += U"{\n";
            ret += U"entry:\n";
            ret += U"    %0 = call i32 @\"" + function_name(fn, ctx) + U"\"(i32 %arg)\n";
            ret += U"    ret i32 %0\n";
            ret += U"}\n\n";
        }

        return ret;
    }
}
}
