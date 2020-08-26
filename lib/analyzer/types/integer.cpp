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

#include "vapor/analyzer/types/integer.h"
#include "vapor/analyzer/expressions/boolean.h"
#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/integer.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/boolean.h"
#include "vapor/codegen/ir/instruction.h"
#include "vapor/codegen/ir/type.h"
#include "vapor/codegen/ir/variable.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    void integer_type::_codegen_type(ir_generation_context &) const
    {
        _codegen_t = codegen::ir::builtin_types().integer;
    }

    std::unique_ptr<type> make_integer_type()
    {
        return std::make_unique<integer_type>();
    }

    template<typename Instruction, typename Eval>
    auto integer_type::_generate_function(const char32_t * name,
        const char * desc,
        Eval eval,
        type * return_type)
    {
        auto lhs = make_runtime_value(builtin_types().integer, nullptr, std::nullopt);
        auto rhs = make_runtime_value(builtin_types().integer, nullptr, std::nullopt);

        auto fun = make_function(desc);
        fun->set_name(name);
        fun->set_return_type(return_type->get_expression());
        fun->set_parameters(unique_expr_list(std::move(lhs), std::move(rhs)));
        fun->set_eval(eval);
        fun->set_intrinsic_codegen(
            [return_type](ir_generation_context & ctx, std::vector<codegen::ir::value> arguments) {
                assert(arguments.size() == 2);
                auto retval = codegen::ir::make_variable(return_type->codegen_type(ctx));

                return statement_ir{ codegen::ir::instruction{ std::nullopt,
                    std::nullopt,
                    { boost::typeindex::type_id<Instruction>() },
                    std::move(arguments),
                    retval } };
            });
        return fun;
    }

#define ADD_OPERATION(NAME, BUILTIN_NAME, OPERATOR, RESULT_TYPE)                                             \
    reaver::vapor::analyzer::_v1::function * reaver::vapor::analyzer::_v1::integer_type::_##NAME()           \
    {                                                                                                        \
        static auto eval = [](auto &&, call_expression * call, const std::vector<expression *> & args) {     \
            assert(args.size() == 2);                                                                        \
            assert(args[0]->get_type() == builtin_types().integer);                                          \
            assert(args[1]->get_type() == builtin_types().integer);                                          \
                                                                                                             \
            if (!args[0]->is_constant() || !args[1]->is_constant())                                          \
            {                                                                                                \
                return make_ready_future<expression *>(nullptr);                                             \
            }                                                                                                \
                                                                                                             \
            auto lhs = args[0]->as<integer_constant>();                                                      \
            auto rhs = args[1]->as<integer_constant>();                                                      \
            return make_ready_future<expression *>(std::make_unique<RESULT_TYPE##_constant>(                 \
                lhs->get_value() OPERATOR rhs->get_value(), call->get_scope(), call->get_name())             \
                                                       .release());                                          \
        };                                                                                                   \
        static auto NAME = _generate_function<codegen::ir::integer_##NAME##_instruction>(                    \
            BUILTIN_NAME, "<builtin integer " #NAME ">", eval, builtin_types().RESULT_TYPE);                 \
        return NAME.get();                                                                                   \
    }

    ADD_OPERATION(addition, U"__builtin_integer_operator_plus", +, integer);
    ADD_OPERATION(subtraction, U"__builtin_integer_operator_minus", -, integer);
    ADD_OPERATION(multiplication, U"__builtin_integer_operator_star", *, integer);
    ADD_OPERATION(equal_comparison, U"__builtin_integer_operator_equals", ==, boolean);
    ADD_OPERATION(less_comparison, U"__builtin_integer_operator_less", <, boolean);
    ADD_OPERATION(less_equal_comparison, U"__builtin_integer_operator_less_equal", <, boolean);
}
}
