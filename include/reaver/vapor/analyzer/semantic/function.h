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
#include <vector>

#include <reaver/function.h>
#include <reaver/optional.h>
#include <reaver/prelude/fold.h>

#include "../../codegen/ir/function.h"
#include "../../print_helpers.h"
#include "../../range.h"
#include "../ir_context.h"
#include "../simplification/context.h"
#include "context.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class type;
    class block;
    class call_expression;

    using function_codegen = reaver::unique_function<codegen::ir::function(ir_generation_context &) const>;
    using intrinsic_codegen =
        reaver::unique_function<statement_ir(ir_generation_context &, std::vector<codegen::ir::value>) const>;
    using function_hook = reaver::unique_function<
        reaver::future<>(analysis_context &, call_expression *, std::vector<expression *>)>;
    using function_eval = reaver::unique_function<
        future<expression *>(recursive_context, call_expression *, std::vector<expression *>)>;

    class function
    {
    public:
        function(std::string explanation, std::optional<range_type> range = std::nullopt)
            : _explanation{ std::move(explanation) }, _range{ std::move(range) }
        {
            auto pair = make_promise<expression *>();
            _return_type_promise = std::move(pair.promise);
            _return_type_future = std::move(pair.future);
        }

        future<expression *> return_type_expression(analysis_context &) const
        {
            return *_return_type_future;
        }

        expression * return_type_expression() const
        {
            assert(_return_type_expression);
            return _return_type_expression;
        }

        const std::vector<expression *> & parameters() const
        {
            return _parameters;
        }

        const std::string & get_explanation() const
        {
            return _explanation;
        }

        std::string explain() const;
        void print(std::ostream & os, print_context ctx, bool print_signature = false) const;

        future<expression *> simplify(recursive_context, call_expression *, std::vector<expression *>);

        void mark_as_entry(analysis_context & ctx, expression * entry_expr)
        {
            assert(!ctx.entry_point_marked);
            ctx.entry_point_marked = true;
            _entry = true;
            _entry_expr = entry_expr;
        }

        codegen::ir::function codegen_ir(ir_generation_context & ctx) const;
        codegen::ir::function_value pointer_ir(ir_generation_context & ctx) const;

        statement_ir intrinsic_codegen_ir(ir_generation_context & ctx,
            std::vector<codegen::ir::value> arguments) const
        {
            assert(_intrinsic_codegen);
            return _intrinsic_codegen.value()(ctx, std::move(arguments));
        }

        void set_return_type(std::shared_ptr<expression> ret)
        {
            _owned_expression = ret;
            set_return_type(_owned_expression.get());
        }

        void set_return_type(expression * ret)
        {
            assert(!_return_type_expression);
            assert(ret);
            _return_type_expression = ret;
            fmap(_return_type_promise, [ret](auto && promise) {
                promise.set(ret);
                return unit{};
            });
        }

        future<expression *> get_return_type() const
        {
            return _return_type_future.value();
        }

        void set_name(std::u32string name)
        {
            _name = name;
        }

        void set_body(block * body)
        {
            _body = body;
        }

        void set_codegen(function_codegen cdg)
        {
            assert(!_codegen && !_intrinsic_codegen);
            _codegen = std::move(cdg);
        }

        void set_intrinsic_codegen(intrinsic_codegen cdg)
        {
            assert(!_codegen && !_intrinsic_codegen);
            _intrinsic_codegen = std::move(cdg);
        }

        block * get_body() const
        {
            return _body;
        }

        void add_analysis_hook(function_hook hook)
        {
            _analysis_hooks.push_back(std::move(hook));
        }

        future<> run_analysis_hooks(analysis_context & ctx,
            call_expression * expr,
            std::vector<expression *> args);

        void set_eval(function_eval eval)
        {
            _compile_time_eval = std::move(eval);
        }

        void set_parameters(std::vector<std::unique_ptr<expression>> params)
        {
            _owned_parameters = std::move(params);
            _parameters = fmap(_owned_parameters, [&](auto && param) { return param.get(); });
        }

        void set_parameters(std::vector<expression *> params)
        {
            _parameters = std::move(params);
        }

        bool is_member() const
        {
            return _is_member;
        }

        void make_member()
        {
            _is_member = true;
        }

        bool is_intrinsic() const
        {
            return _intrinsic_codegen.has_value();
        }

        std::optional<std::size_t> vtable_slot() const
        {
            return _vtable_id;
        }

        void mark_virtual(std::size_t id)
        {
            assert(!_vtable_id);
            _vtable_id = id;
        }

        void mark_exported()
        {
            _is_exported = true;
        }

        auto & get_range() const
        {
            return _range;
        }

    private:
        std::string _explanation;
        std::optional<range_type> _range;

        block * _body = nullptr;
        expression * _return_type_expression = nullptr;
        std::optional<future<expression *>> _return_type_future;
        std::optional<manual_promise<expression *>> _return_type_promise;
        // this is shared ONLY because unique_ptr would require the definition of `expression`
        std::shared_ptr<expression> _owned_expression;

        bool _is_member = false;
        bool _is_intrinsic = false;
        bool _is_exported = false;
        std::optional<std::size_t> _vtable_id;
        std::vector<std::unique_ptr<expression>> _owned_parameters;
        std::vector<expression *> _parameters;
        std::optional<std::u32string> _name;
        std::optional<function_codegen> _codegen;
        std::optional<intrinsic_codegen> _intrinsic_codegen;
        mutable std::optional<codegen::ir::function> _ir;

        std::vector<function_hook> _analysis_hooks;
        std::optional<function_eval> _compile_time_eval;

        bool _entry = false;
        expression * _entry_expr = nullptr;
    };

    inline std::unique_ptr<function> make_function(std::string expl,
        std::optional<range_type> range = std::nullopt)
    {
        return std::make_unique<function>(std::move(expl), std::move(range));
    }
}
}
