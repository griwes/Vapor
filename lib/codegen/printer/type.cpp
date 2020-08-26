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
    std::u32string ir_printer::generate_definition(std::shared_ptr<ir::type> type,
        codegen::codegen_context & ctx)
    {
        if (type->is_fundamental())
        {
            return U"";
        }

        if (auto user = dynamic_cast<ir::user_type *>(type.get()))
        {
            std::u32string members;

            fmap(user->members, [&](auto && member) {
                fmap(member,
                    make_overload_set(
                        [&](codegen::ir::function & fn) {
                            auto old_generated = ctx.declaring_members_for;
                            ctx.declaring_members_for = type;

                            if (!fn.is_member)
                            {
                                members += U"[nonmember] ";
                            }
                            members += this->generate_definition(fn, ctx);

                            ctx.declaring_members_for = old_generated;

                            return unit{};
                        },
                        [&](codegen::ir::member_variable & member) {
                            members += this->generate_definition(member, ctx);
                            return unit{};
                        },
                        [&](auto &&) {
                            assert(0);
                            return unit{};
                        }));
                return unit{};
            });

            return U"define user @ " + _pointer_to_string(type.get()) + U" `" + U"." + user->name + U"`:\n{\n"
                + members + U"}\n";
        }

        assert(!"unsupported type in codegen ir!");
    }
}
}
