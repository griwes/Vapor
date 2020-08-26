/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2020 Michał "Griwes" Dominiak
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

#include <reaver/mayfly.h>

#include "vapor/analyzer.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/lexer.h"
#include "vapor/parser.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    template<typename F>
    auto parse(std::u32string program, F && parser)
    {
        parser::context ctx;
        ctx.begin = lexer::iterator{ program.begin(), program.end(), std::nullopt };

        return parser(ctx);
    }

    class unexpected_call : public reaver::exception
    {
    public:
        unexpected_call(const char * desc) : exception{ reaver::logger::fatal }
        {
            *this << desc;
        }
    };

    class test_expression : public expression
    {
    public:
        test_expression(type * t = nullptr) : expression{ t, nullptr, std::nullopt }
        {
        }

        virtual void print(std::ostream &, print_context) const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

        void set_analysis_type(type * t)
        {
            _analysis_type = t;
        }

        void set_clone_result(std::unique_ptr<expression> expr)
        {
            _cloned_expr = std::move(expr);
        }

        void set_simplified_expression(std::unique_ptr<expression> expr)
        {
            _simplified_expr = std::move(expr);
        }

    private:
        virtual reaver::future<> _analyze(analysis_context &) override
        {
            if (!try_get_type())
            {
                if (!_analysis_type)
                {
                    throw unexpected_call{ __PRETTY_FUNCTION__ };
                }

                _set_type(std::move(_analysis_type));
            }

            return reaver::make_ready_future();
        }

        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override
        {
            if (!_cloned_expr)
            {
                throw unexpected_call{ __PRETTY_FUNCTION__ };
            }

            return std::move(const_cast<test_expression *>(this)->_cloned_expr);
        }

        virtual reaver::future<expression *> _simplify_expr(recursive_context) override
        {
            if (!_simplified_expr)
            {
                throw unexpected_call{ __PRETTY_FUNCTION__ };
            }

            return make_ready_future(_simplified_expr.release());
        }

        virtual statement_ir _codegen_ir(ir_generation_context &) const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

        virtual bool _is_equal(const expression * rhs) const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

        type * _analysis_type = nullptr;
        std::unique_ptr<expression> _cloned_expr;
        std::unique_ptr<expression> _simplified_expr;
    };

    class test_type : public type
    {
    public:
        test_type() = default;

        virtual std::string explain() const override
        {
            return "test type";
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::rule_name << "test-type\n";
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            assert(0);
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            assert(0);
        }

        virtual std::u32string codegen_name() const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override
        {
            throw unexpected_call{ __PRETTY_FUNCTION__ };
        }
    };
}
}
