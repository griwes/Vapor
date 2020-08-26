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

#pragma once

#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    inline std::unique_ptr<expression> make_conversion_expression(std::unique_ptr<expression> expr,
        type * conv,
        scope * lex_scope,
        std::optional<std::u32string> name);

    class conversion_expression : public expression
    {
    public:
        conversion_expression(expression * expr,
            type * conv,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : expression{ conv, lex_scope, std::move(name) }, _base{ expr }
        {
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "conversion-expression";
            os << styles::def << " @ " << styles::address << this << styles::def << ":\n";

            auto target_ctx = ctx.make_branch(false);
            os << styles::def << target_ctx << styles::subrule_name << "target type:\n";
            get_type()->print(os, target_ctx.make_branch(true));

            auto base_ctx = ctx.make_branch(true);
            os << styles::def << base_ctx << styles::subrule_name << "base expression:\n";
            _base->print(os, base_ctx.make_branch(true));
        }

    private:
        virtual future<> _analyze(analysis_context &) override
        {
            return make_ready_future();
        }

    protected:
        virtual future<expression *> _simplify_expr(recursive_context ctx) override
        {
            return _base->simplify_expr(ctx).then([&, this](auto && simplified) -> expression * {
                if (simplified)
                {
                    _base = simplified;
                }
                _base = _base->_get_replacement();

                if (_base->is_constant())
                {
                    if (auto converted = _base->convert_to(this->get_type()))
                    {
                        return converted.release();
                    }
                }

                return this;
            });
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return make_conversion_expression(repl.copy_claim(_base), get_type(), get_scope(), get_name());
        }

        virtual statement_ir _codegen_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        expression * _base;
    };

    class owning_conversion_expression : public conversion_expression
    {
    public:
        owning_conversion_expression(std::unique_ptr<expression> expr,
            type * target,
            scope * lex_scope,
            std::optional<std::u32string> name)
            : conversion_expression{ expr.get(), target, lex_scope, std::move(name) },
              _owned{ std::move(expr) }
        {
        }

    private:
        virtual future<expression *> _simplify_expr(recursive_context ctx) override
        {
            return _owned->simplify_expr(ctx).then(
                [&, this, ctx](auto && simplified) -> future<expression *> {
                    replace_uptr(_owned, simplified, ctx.proper);
                    return this->conversion_expression::_simplify_expr(ctx).then(
                        [&](auto && simpl) -> expression * {
                            if (simpl && simpl != this)
                            {
                                return simpl;
                            }
                            return this;
                        });
                });
        }

        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return make_conversion_expression(repl.claim(_owned.get()), get_type(), get_scope(), get_name());
        }

        std::unique_ptr<expression> _owned;
    };

    inline auto make_conversion_expression(expression * expr,
        type * conv,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        return std::make_unique<conversion_expression>(expr, conv, lex_scope, std::move(name));
    }

    inline std::unique_ptr<expression> make_conversion_expression(std::unique_ptr<expression> expr,
        type * conv,
        scope * lex_scope,
        std::optional<std::u32string> name)
    {
        return std::make_unique<owning_conversion_expression>(
            std::move(expr), conv, lex_scope, std::move(name));
    }
}
}
