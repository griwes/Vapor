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

#include "vapor/analyzer/types/type.h"

#include <reaver/id.h>

#include "vapor/analyzer/expressions/call.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/expressions/type.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/function.h"
#include "vapor/analyzer/semantic/overloads.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/types/archetype.h"
#include "vapor/analyzer/types/pack.h"
#include "vapor/analyzer/types/sized_integer.h"
#include "vapor/analyzer/types/unconstrained.h"

#include "expressions/type.pb.h"
#include "type_reference.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    type::type() : _member_scope{ nullptr }
    {
        _init_expr();
        _init_pack_type();
    }

    type::type(type::dont_init_expr_t) : _member_scope{ nullptr }
    {
    }

    type::type(std::unique_ptr<scope> member_scope) : _member_scope{ std::move(member_scope) }
    {
        _init_expr();
        _init_pack_type();
    }

    type::type(type::dont_init_expr_t, std::unique_ptr<scope> member_scope)
        : _member_scope{ std::move(member_scope) }
    {
    }

    void type::init_expr()
    {
        if (!_self_expression)
        {
            if (!_expression_initialization)
            {
                _expression_initialization = true;
                _init_expr();
                _init_pack_type();
            }
        }
    }

    type::type(type::dont_init_pack_t) : _member_scope{ nullptr }
    {
        _init_expr();
    }

    type::~type() = default;

    void type::_init_expr()
    {
        _self_expression = make_type_expression(this);
    }

    void type::_init_pack_type()
    {
        _pack_type = make_pack_type(this);
    }

    future<std::vector<function *>> type::get_candidates(lexer::token_type) const
    {
        return make_ready_future(std::vector<function *>{});
    }

    future<function *> type::get_constructor(std::vector<const expression *>) const
    {
        return make_ready_future(static_cast<function *>(nullptr));
    }

    type * type::get_member_type(const std::u32string &) const
    {
        return nullptr;
    }

    expression * type::get_expression() const
    {
        return _self_expression->_get_replacement();
    }

    type * type::get_pack_type() const
    {
        assert(_pack_type);
        return _pack_type.get();
    }

    bool type::matches(type * other) const
    {
        return this == other;
    }

    bool type::matches(const std::vector<type *> & types) const
    {
        return false;
    }

    bool type::needs_conversion(type * other) const
    {
        return false;
    }

    bool type::is_meta() const
    {
        return false;
    }

    std::unique_ptr<archetype> type::generate_archetype(ast_node node, std::u32string param_name) const
    {
        assert(!is_meta() && "default generate_archetype hit, even though is_meta() is true");
        assert(!"generate_archetype invoked on a non-meta type");
    }

    class type_type : public type
    {
    public:
        type_type() : type{ dont_init_expr }
        {
        }

        virtual std::string explain() const override
        {
            return "type";
        }

        virtual future<std::vector<function *>> get_candidates(lexer::token_type token) const override
        {
            if (token != lexer::token_type::curly_bracket_open)
            {
                assert(0);
                return make_ready_future(std::vector<function *>{});
            }

            [&] {
                if (_generic_ctor)
                {
                    return;
                }

                _generic_ctor_first_arg = make_runtime_value(builtin_types().type, nullptr, std::nullopt);
                _generic_ctor_pack_arg =
                    make_runtime_value(builtin_types().unconstrained->get_pack_type(), nullptr, std::nullopt);

                _generic_ctor = make_function("generic constructor");
                _generic_ctor->set_return_type(_generic_ctor_first_arg.get());
                _generic_ctor->set_parameters(
                    { _generic_ctor_first_arg.get(), _generic_ctor_pack_arg.get() });

                _generic_ctor->make_member();

                _generic_ctor->add_analysis_hook([](auto && ctx, auto && call_expr, auto && args) {
                    assert(args.size() != 0);
                    assert(args.front()->get_type() == builtin_types().type);
                    assert(args.front()->is_constant());

                    auto type_expr = args.front()->template as<type_expression>();
                    auto actual_type = type_expr->get_value();
                    args.erase(args.begin());
                    auto actual_ctor = actual_type->get_constructor(
                        fmap(args, [](auto && arg) -> const expression * { return arg; }));

                    return actual_ctor
                        .then([&ctx, args, call_expr](auto && ctor) {
                            return select_overload(ctx,
                                call_expr->get_range(),
                                call_expr->get_scope(),
                                call_expr->get_name(),
                                args,
                                { ctor });
                        })
                        .then([call_expr](auto && expr) { call_expr->replace_with(std::move(expr)); });
                });

                _generic_ctor->set_eval([](auto &&, call_expression *, auto &&) -> future<expression *> {
                    assert(!"a generic constructor call survived analysis; this is a compiler bug");
                });
            }();

            return make_ready_future(std::vector<function *>{ _generic_ctor.get() });
        }

        virtual void print(std::ostream & os, print_context ctx) const override
        {
            os << styles::def << ctx << styles::type << "type" << styles::def << " @ " << styles::address
               << this << styles::def << ": builtin type\n";
        }

        virtual bool is_meta() const override
        {
            return true;
        }

        virtual std::unique_ptr<archetype> generate_archetype(ast_node node,
            std::u32string param_name) const override
        {
            auto arch = std::make_unique<archetype>(node, this, std::move(param_name));
            return arch;
        }

        virtual std::unique_ptr<proto::type> generate_interface() const override
        {
            auto ret = std::make_unique<proto::type>();
            ret->set_allocated_reference(generate_interface_reference().release());
            return ret;
        }

        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const override
        {
            auto ret = std::make_unique<proto::type_reference>();
            ret->set_builtin(proto::type_);
            return ret;
        }

        virtual std::u32string codegen_name() const override
        {
            return U"type";
        }

    private:
        virtual void _codegen_type(ir_generation_context &) const override
        {
            assert(0);
        }

        mutable std::shared_ptr<function> _generic_ctor;
        mutable std::unique_ptr<expression> _generic_ctor_first_arg;
        mutable std::unique_ptr<expression> _generic_ctor_pack_arg;
    };

    std::unique_ptr<type> make_type_type()
    {
        return std::make_unique<type_type>();
    }

    std::unique_ptr<proto::type> user_defined_type::generate_interface() const
    {
        auto ret = std::make_unique<proto::type>();

        auto message = _user_defined_interface();

        auto dynamic_switch = [&](auto &&... pairs) {
            ((dynamic_cast<typename decltype(pairs.first)::type *>(message.get())
                 && pairs.second(static_cast<typename decltype(pairs.first)::type *>(message.release())))
                    || ... || [&]() -> bool {
                auto m = message.get();
                throw exception{ logger::crash } << "unhandled serialized type type: `" << typeid(*m).name()
                                                 << "`";
            }());
        };

#define HANDLE_TYPE(type, field_name)                                                                        \
    std::make_pair(id<proto::type>(), [&](auto ptr) {                                                        \
        ret->set_allocated_##field_name(ptr);                                                                \
        return true;                                                                                         \
    })

        dynamic_switch(HANDLE_TYPE(overload_set_type, overload_set), HANDLE_TYPE(struct_type, struct_));

#undef HANDLE_TYPE

        return ret;
    }

    std::unique_ptr<proto::type_reference> user_defined_type::generate_interface_reference() const
    {
        auto user_defined = std::make_unique<proto::user_defined_reference>();

        std::vector<scope *> scope_stack;
        auto current = get_scope()->parent();
        while (current)
        {
            scope_stack.push_back(current);
            current = current->parent();
        }

        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
        {
            auto scope = *it;
            switch (scope->get_kind())
            {
                case scope_kind::module:
                    break;

                case scope_kind::type:
                    assert(!"should probably implemented nested UDT references now... ;)");

                case scope_kind::global:
                    break;

                default:
                    assert(0);
            }
        }

        user_defined->set_name(utf8(get_scope()->get_entity_name()));

        auto ret = std::make_unique<proto::type_reference>();
        ret->set_allocated_user_defined(user_defined.release());

        return ret;
    }

    void user_defined_type::_codegen_type(ir_generation_context & ctx) const
    {
        if (!_codegen_t)
        {
            auto user_type = std::make_shared<codegen::ir::user_type>();
            _codegen_t = user_type;
            _codegen_user_type(ctx, std::move(user_type));
        }
    }

    std::unique_ptr<type> make_type_type();
    std::unique_ptr<type> make_integer_type();
    std::unique_ptr<type> make_boolean_type();
    std::unique_ptr<type> make_unconstrained_type();

    const builtin_types_t<type *> & builtin_types()
    {
        static auto builtins = [] {
            builtin_types_t<std::unique_ptr<type>> ret;

            ret.type = make_type_type();
            ret.integer = make_integer_type();
            ret.boolean = make_boolean_type();
            ret.unconstrained = make_unconstrained_type();

            return ret;
        }();

        static auto builtins_raw = [&] {
            builtin_types_t<type *> ret;

            ret.type = builtins.type.get();
            ret.integer = builtins.integer.get();
            ret.boolean = builtins.boolean.get();
            ret.unconstrained = builtins.unconstrained.get();

            return ret;
        }();

        builtins.type->init_expr();
        builtins.integer->init_expr();
        builtins.boolean->init_expr();
        builtins.unconstrained->init_expr();

        return builtins_raw;
    }
}
}
