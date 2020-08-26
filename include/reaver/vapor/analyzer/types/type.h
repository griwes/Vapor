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

#include <reaver/future.h>
#include <reaver/variant.h>

#include <google/protobuf/message.h>

#include "../../codegen/ir/type.h"
#include "../../lexer/token.h"
#include "../../print_helpers.h"
#include "../ir_context.h"
#include "../semantic/scope.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    namespace ir
    {
        struct type;
    }
}
}

namespace reaver::vapor::proto
{
struct type;
struct type_reference;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class function;
    class expression;
    class archetype;

    class type
    {
    public:
        static constexpr struct dont_init_expr_t
        {
        } dont_init_expr{};

        type();
        type(dont_init_expr_t);
        type(std::unique_ptr<scope> member_scope);
        type(dont_init_expr_t, std::unique_ptr<scope> lex_scope);

        void init_expr();

    protected:
        // this is virtually only for `pack_type`
        // don't abuse, please
        static constexpr struct dont_init_pack_t
        {
        } dont_init_pack{};

        type(dont_init_pack_t);

    public:
        virtual ~type();

        virtual future<std::vector<function *>> get_candidates(lexer::token_type) const;
        virtual future<function *> get_constructor(std::vector<const expression *>) const;
        virtual type * get_member_type(const std::u32string &) const;

        virtual std::string explain() const = 0;
        virtual void print(std::ostream & os, print_context ctx) const = 0;

        scope * get_scope() const
        {
            return _member_scope.get();
        }

        std::shared_ptr<codegen::ir::type> codegen_type(ir_generation_context & ctx) const
        {
            if (!_codegen_t)
            {
                _codegen_type(ctx);
            }
            return *_codegen_t;
        }

        expression * get_expression() const;

        virtual type * get_pack_type() const;
        virtual bool matches(type * other) const;
        virtual bool matches(const std::vector<type *> & types) const;
        virtual bool needs_conversion(type * other) const;
        virtual bool is_meta() const;
        virtual std::unique_ptr<archetype> generate_archetype(ast_node node, std::u32string param_name) const;

        virtual std::u32string codegen_name() const = 0;

        virtual std::unique_ptr<proto::type> generate_interface() const = 0;
        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const = 0;

    private:
        virtual void _codegen_type(ir_generation_context &) const = 0;

    protected:
        std::unique_ptr<scope> _member_scope;
        // only shared to not require a complete definition of expression to be visible
        // (unique_ptr would require that unless I moved all ctors and dtors out of the header)
        bool _expression_initialization = false;
        std::shared_ptr<expression> _self_expression;
        void _init_expr();
        std::unique_ptr<type> _pack_type;
        void _init_pack_type();

        mutable std::optional<std::shared_ptr<codegen::ir::type>> _codegen_t;

        std::u32string _name;
    };

    class user_defined_type : public type
    {
    public:
        using type::type;

        virtual std::unique_ptr<proto::type> generate_interface() const final;
        virtual std::unique_ptr<proto::type_reference> generate_interface_reference() const final;

        virtual std::u32string codegen_name() const final
        {
            assert(_member_scope);
            return std::u32string{ _member_scope->get_entity_name() };
        }

    private:
        virtual std::unique_ptr<google::protobuf::Message> _user_defined_interface() const = 0;
        virtual void _codegen_type(ir_generation_context &) const final;
        virtual void _codegen_user_type(ir_generation_context &,
            std::shared_ptr<codegen::ir::user_type>) const = 0;
    };

    template<typename Pointer>
    struct builtin_types_t
    {
        Pointer type;
        Pointer integer;
        Pointer boolean;
        Pointer unconstrained;
    };

    const builtin_types_t<type *> & builtin_types();
}
}
