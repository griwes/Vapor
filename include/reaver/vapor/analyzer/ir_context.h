/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2020 Michał "Griwes" Dominiak
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
#include <unordered_set>
#include <vector>

#include <reaver/logger.h>

#include "../codegen/ir/entity.h"
#include "../codegen/ir/function.h"
#include "../codegen/ir/instruction.h"
#include "../codegen/ir/variable.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function;
    class expression;

    using statement_ir = std::vector<codegen::ir::instruction>;
    using declaration_ir = std::vector<codegen::ir::entity>;
    using constant_init_ir = codegen::ir::value;

    class ir_generation_context
    {
    public:
        void add_function_to_generate(const function * fn);
        void add_generated_function(const function * fn);
        const function * function_to_generate();

        bool top_level_generation = true;
        std::size_t label_index = 0;

        void push_base_expression(const expression * expr)
        {
            _base_expression_stack.push_back(expr);
        }

        void pop_base_expression(const expression * expr)
        {
            assert(!_base_expression_stack.empty());
            assert(_base_expression_stack.back() == expr);
            _base_expression_stack.pop_back();
        }

        const expression * get_current_base() const
        {
            assert(!_base_expression_stack.empty());
            return _base_expression_stack.back();
        }

    private:
        std::vector<const function *> _functions_to_generate;
        std::unordered_set<const function *> _generated_functions;
        std::vector<const expression *> _base_expression_stack;
    };
}
}
