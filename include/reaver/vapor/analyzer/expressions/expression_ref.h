/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2018 Michał "Griwes" Dominiak
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

#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class expression_ref : public expression
    {
    protected:
        expression_ref() = default;

    public:
        expression_ref(expression * expr) : _referenced{ expr }
        {
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "expression-ref";
            os << styles::def << " @ " << styles::address << this << styles::def << ":\n";

            auto expr_ctx = ctx.make_branch(false);
            os << styles::def << expr_ctx << styles::subrule_name << "referenced expression";
            os << styles::def << " @ " << styles::address << _referenced << '\n';

            auto type_ctx = ctx.make_branch(true);
            os << styles::def << type_ctx << styles::subrule_name << "referenced expression type:\n";
            get_type()->print(os, type_ctx.make_branch(true));
        }

    private:
        virtual expression * _get_replacement() override
        {
            return _referenced->_get_replacement();
        }

        virtual const expression * _get_replacement() const override
        {
            return _referenced->_get_replacement();
        }

        virtual future<> _analyze(analysis_context & ctx) override
        {
            return _referenced->analyze(ctx).then([&] {
                _referenced = _referenced->_get_replacement();

                if (auto type = _referenced->try_get_type())
                {
                    _set_type(type);
                }
            });
        }

        virtual std::unique_ptr<expression> _clone_expr_with_replacement(replacements & repl) const override
        {
            auto referenced = _referenced;

            if (auto replaced = repl.try_get_replacement(referenced->_get_replacement()))
            {
                referenced = replaced;
            }

            auto ret = std::make_unique<expression_ref>(referenced);
            if (auto type = try_get_type())
            {
                ret->_set_type(type);
            }
            return ret;
        }

        virtual future<expression *> _simplify_expr(recursive_context ctx) override
        {
            return _referenced->simplify_expr(ctx).then([&](auto && simplified) -> expression * {
                if (simplified && simplified != _referenced)
                {
                    _referenced = simplified;
                }
                _referenced = _referenced->_get_replacement();

                return this;
            });
        }

        virtual statement_ir _codegen_ir(ir_generation_context & ctx) const override
        {
            auto referenced_ir = _referenced->codegen_ir(ctx);
            return { codegen::ir::instruction{
                std::nullopt, std::nullopt, { boost::typeindex::type_id<codegen::ir::pass_value_instruction>() }, {}, { referenced_ir.back().result } } };
        }

        virtual bool _is_equal(const expression * rhs) const override
        {
            return _referenced->is_equal(rhs);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

    protected:
        expression * _referenced = nullptr;
    };

    inline std::unique_ptr<expression> make_expression_ref(expression * expr)
    {
        return std::make_unique<expression_ref>(expr);
    }
}
}
