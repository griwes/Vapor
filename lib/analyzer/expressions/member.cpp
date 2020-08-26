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

#include "vapor/analyzer/expressions/member.h"
#include "vapor/analyzer/semantic/symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    codegen::ir::member_variable member_expression::member_codegen_ir(ir_generation_context & ctx) const
    {
        return codegen::ir::member_variable{ get_name().value(), get_type()->codegen_type(ctx) };
    }
}
}
