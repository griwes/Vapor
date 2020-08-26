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

#include "vapor/codegen/llvm_ir.h"

#include <boost/algorithm/string/join.hpp>

#include "vapor/codegen/ir/entity.h"
#include "vapor/codegen/ir/type.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    std::u32string llvm_ir_generator::generate_global_definitions(codegen_context &) const
    {
        return UR"code(target triple = "x86_64-pc-linux-gnu"

)code";
    }

    std::u32string llvm_ir_generator::generate_definitions(std::vector<ir::entity> & module,
        codegen_context & ctx)
    {
        std::u32string ret;

        for (auto && entity : module)
        {
            ret += std::get<0>(fmap(entity,
                make_overload_set(
                    [&](std::shared_ptr<ir::variable> & var) { return this->generate_definition(*var, ctx); },
                    [&](ir::function & fn) { return this->generate_definition(fn, ctx); })));
        }

        return ret;
    }

    std::u32string llvm_ir_generator::type_name(std::shared_ptr<ir::type> type, codegen_context & ctx)
    {
        if (type == ir::builtin_types().integer)
        {
            assert(0);
        }

        if (type == ir::builtin_types().boolean)
        {
            return U"i1";
        }

        if (auto sized = dynamic_cast<const ir::sized_integer_type *>(type.get()))
        {
            return U"i" + utf32(std::to_string(sized->integer_size));
        }

        if (auto function = dynamic_cast<const ir::function_type *>(type.get()))
        {
            return type_name(function->return_type, ctx) + U" ("
                + boost::join(fmap(function->parameter_types,
                                  [&](auto && param_type) { return type_name(param_type, ctx); }),
                    U", ")
                + U") *"; // TODO: this decay to pointer to function should probably happen earlier?
        }

        if (auto user = dynamic_cast<const ir::user_type *>(type.get()))
        {
            ctx.put_into_global_before += ctx.define_if_necessary(type);
            return U"%\"" + user->name + U"\"";
        }

        assert(!"unsupported type in codegen ir!");
    }

    std::u32string llvm_ir_generator::function_name(ir::function & fn, codegen_context & ctx)
    {
        return fn.name;
    }

    std::u32string llvm_ir_generator::variable_name(ir::variable & var, codegen_context & ctx)
    {
        if (!var.name)
        {
            var.name = utf32(std::to_string(ctx.unnamed_variable_index++));
        }

        return (ctx.in_function_definition ? U"%\"" : U"@\"") + var.name.value() + U"\"";
    }

    std::u32string llvm_ir_generator::variable_of(const ir::value & val, codegen_context & ctx)
    {
        assert(val.index() == 0);
        return variable_name(*std::get<std::shared_ptr<ir::variable>>(val), ctx);
    }

    std::u32string llvm_ir_generator::type_of(const ir::value & val, codegen_context & ctx)
    {
        return std::get<std::u32string>(fmap(val,
            make_overload_set(
                [&](const ir::integer_value & val) {
                    assert(val.size);

                    std::ostringstream os;
                    os << "i" << val.size.value();
                    return utf32(os.str());
                },
                [&](const ir::boolean_value &) -> std::u32string { return U"i1"; },
                [&](const std::shared_ptr<ir::variable> & var) {
                    ctx.put_into_global_before += ctx.define_if_necessary(var->type);
                    return type_name(var->type, ctx);
                },
                [&](const ir::label &) {
                    assert(0);
                    return unit{};
                },
                [&](const ir::struct_value & val) {
                    ctx.put_into_global_before += ctx.define_if_necessary(val.type);
                    return type_name(val.type, ctx);
                },
                [&](const ir::function_value & val) { return type_name(val.type, ctx); },
                [](auto &&) {
                    assert(0);
                    return unit{};
                })));
    }

    std::u32string llvm_ir_generator::value_of(const ir::value & val, codegen_context & ctx)
    {
        return std::get<std::u32string>(fmap(val,
            make_overload_set(
                [&](const ir::integer_value & val) {
                    std::ostringstream os;
                    os << val.value;
                    return utf32(os.str());
                },
                [&](const ir::boolean_value & val) -> std::u32string {
                    return val.value ? U"true" : U"false";
                },
                [&](const std::shared_ptr<ir::variable> & var) { return variable_name(*var, ctx); },
                [&](const ir::label & label) { return U"%\"" + label.name + U"\""; },
                [&](const ir::struct_value & val) {
                    std::u32string ret;
                    std::u32string indent(ctx.nested_indent, U' ');

                    ret += U"{";

                    ctx.nested_indent += 2;

                    for (auto && member : val.fields)
                    {
                        ret += U"\n  " + indent + type_of(member, ctx) + U" " + value_of(member, ctx) + U",";
                    }

                    ctx.nested_indent -= 2;

                    if (ret.back() == U',')
                    {
                        ret.pop_back();
                    }
                    ret += U"\n" + indent + U"}";

                    return ret;
                },
                [&](const ir::function_value & val) { return U"@\"" + val.name + U"\""; },
                [&](auto &&) {
                    assert(0);
                    return unit{};
                })));
    }
}
}
