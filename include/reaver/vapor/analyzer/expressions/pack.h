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

#include "../types/pack.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::unique_ptr<expression> make_pack_expression(std::vector<std::unique_ptr<expression>> vars,
        type * pack_type = builtin_types().type->get_pack_type());

    class pack_expression : public expression
    {
    public:
        pack_expression(std::vector<expression *> exprs, type * pack_type)
            : expression{ nullptr, std::nullopt }, _exprs{ std::move(exprs) }, _type{ pack_type }
        {
            _set_type(_type);
        }

        virtual void print(std::ostream &, print_context) const override
        {
            assert(0);
        }

    private:
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return make_pack_expression(fmap(_exprs, [&](auto && expr) { return repl.claim(expr); }), _type);
        }

        virtual statement_ir _codegen_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual bool _is_equal(const expression * rhs) const override
        {
            auto rhs_pack = rhs->as<pack_expression>();
            return _exprs.size() == rhs_pack->_exprs.size()
                && std::equal(
                    _exprs.begin(), _exprs.end(), rhs_pack->_exprs.begin(), [](auto && lhs, auto && rhs) {
                        return lhs->is_equal(rhs);
                    });
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        std::vector<expression *> _exprs;
        type * _type;
    };

    class owning_pack_expression : public pack_expression
    {
    public:
        owning_pack_expression(std::vector<std::unique_ptr<expression>> vars, type * pack_type)
            : pack_expression{ fmap(vars, [](auto && var) { return var.get(); }), pack_type },
              _vars{ std::move(vars) }
        {
        }

    private:
        std::vector<std::unique_ptr<expression>> _vars;
    };

    inline auto make_pack_expression(std::vector<expression *> vars = {},
        type * pack_type = builtin_types().type->get_pack_type())
    {
        return std::make_unique<pack_expression>(std::move(vars), pack_type);
    }

    inline std::unique_ptr<expression> make_pack_expression(std::vector<std::unique_ptr<expression>> vars,
        type * pack_type)
    {
        return std::make_unique<owning_pack_expression>(std::move(vars), pack_type);
    }
}
}
