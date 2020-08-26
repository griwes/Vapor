/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2019 Michał "Griwes" Dominiak
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

#include <boost/multiprecision/integer.hpp>

#include "../../parser/literal.h"
#include "constant.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class integer_constant : public constant
    {
    public:
        integer_constant(boost::multiprecision::cpp_int value,
            scope * lex_scope,
            std::optional<std::u32string> name,
            ast_node parse = {})
            : constant{ builtin_types().integer, lex_scope, std::move(name) }, _value{ std::move(value) }
        {
            _set_ast_info(parse);
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "integer-constant";
            print_address_range(os, this);
            os << ' ' << styles::string_value << _value << '\n';
        }

        const auto & get_value() const
        {
            return _value;
        }

        virtual std::unique_ptr<expression> convert_to(type * target) const override;

    private:
        virtual future<> _analyze(analysis_context &) override
        {
            return make_ready_future();
        }

        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override
        {
            return std::make_unique<integer_constant>(
                _value, get_scope(), get_name(), get_ast_info().value());
        }

        virtual future<expression *> _simplify_expr(recursive_context) override
        {
            return make_ready_future<expression *>(this);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        virtual bool _is_equal(const expression * rhs) const override
        {
            auto rhs_int = rhs->as<integer_constant>();
            return rhs_int && _value == rhs_int->_value;
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            assert(0);
        }

        boost::multiprecision::cpp_int _value;
    };

    inline std::unique_ptr<integer_constant> make_integer_constant(const parser::integer_literal & parse,
        scope * lex_scope,
        std::optional<std::u32string> name = std::nullopt)
    {
        return std::make_unique<integer_constant>(boost::multiprecision::cpp_int{ utf8(parse.value.string) },
            lex_scope,
            std::move(name),
            make_node(parse));
    }
}
}
