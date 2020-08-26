/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2019 Michał "Griwes" Dominiak
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

#include <google/protobuf/any.pb.h>

#include <reaver/prelude/monad.h>

#include "../helpers.h"
#include "../statements/statement.h"
#include "../types/type.h"
#include "context.h"

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct expression;
}
}

namespace reaver::vapor::proto
{
struct entity;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class scope;

    constexpr struct imported_tag_t
    {
    } imported_tag;

    class expression : public statement
    {
    public:
        virtual ~expression() = default;

        expression(scope * lex_scope, std::optional<std::u32string> name)
            : _lex_scope{ lex_scope }, _name{ std::move(name) }
        {
        }

        expression(type * t, scope * lex_scope, std::optional<std::u32string> name)
            : _type{ t }, _lex_scope{ lex_scope }, _name{ std::move(name) }
        {
        }

        type * get_type() const
        {
            if (!_type)
            {
                assert(!"tried to get an unset type");
            }

            return _type;
        }

        type * try_get_type() const
        {
            return _type;
        }

        friend class replacements;

    private:
        std::unique_ptr<expression> clone_expr(replacements & repl) const
        {
            return _clone_expr(repl);
        }

    public:
        future<expression *> simplify_expr(recursive_context ctx);

        void set_context(expression_context ctx)
        {
            _expr_ctx = std::move(ctx);
        }

        const expression_context & get_context() const
        {
            return _expr_ctx;
        }

        expression * get_default_value() const
        {
            return _default_value;
        }

        void set_default_value(expression * expr)
        {
            assert(!_default_value);
            _default_value = expr;
        }

        virtual bool is_constant() const;
        bool is_equal(const expression * rhs) const;
        bool is_different_constant(const expression * rhs);
        virtual std::unique_ptr<expression> convert_to(type * target) const;
        virtual bool is_member() const;
        virtual bool is_member_assignment() const;
        virtual bool is_member_access() const;
        virtual expression * get_member(const std::u32string & name) const;
        virtual function * get_vtable_entry(std::size_t) const;

        const std::optional<std::u32string> & get_name() const
        {
            return _name;
        }

        scope * get_scope() const
        {
            return _lex_scope;
        }

        std::optional<std::u32string_view> get_entity_name() const;

        template<typename T>
        T * as()
        {
            return dynamic_cast<T *>(_get_replacement());
        }

        template<typename T>
        const T * as() const
        {
            return dynamic_cast<const T *>(_get_replacement());
        }

        virtual std::size_t hash_value() const
        {
            return 0;
        }

        // this ought to be protected
        // but then derived classes wouldn't be able to recurse
        // so let's just mark it as "protected interface"
        // these aren't mutators, so nothing bad should ever happen
        // but I know these are famous last words
        virtual expression * _get_replacement()
        {
            return this;
        }

        virtual const expression * _get_replacement() const
        {
            return this;
        }

        void generate_interface(proto::entity &) const;

        std::unique_ptr<google::protobuf::Message> _do_generate_interface() const
        {
            return _generate_interface();
        }

        virtual std::unordered_set<expression *> get_associated_entities() const
        {
            return {};
        }

        virtual void mark_exported()
        {
        }

        constant_init_ir constinit_ir(ir_generation_context & ctx) const
        {
            assert(is_constant());
            return _constinit_ir(ctx);
        }

    protected:
        void _set_type(type * t)
        {
            assert((!_type) ^ (_type == t));
            _type = t;
        }

        void _set_scope(scope * s)
        {
            assert(!_lex_scope);
            _lex_scope = s;
        }

        virtual std::optional<std::u32string> _get_entity_name() const;
        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<statement> _clone(replacements & repl) const final;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const = 0;
        virtual future<statement *> _simplify(recursive_context ctx) override final;
        virtual future<expression *> _simplify_expr(recursive_context);
        virtual constant_init_ir _constinit_ir(ir_generation_context & ctx) const = 0;
        virtual bool _is_equal([[maybe_unused]] const expression * expr) const;
        virtual bool _is_pure() const;
        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const;

    private:
        type * _type = nullptr;
        expression * _default_value = nullptr;
        scope * _lex_scope = nullptr;
        std::optional<std::u32string> _name;
        mutable std::optional<std::u32string> _entity_name; // this is computed and cached

        expression_context _expr_ctx;
    };

    inline std::size_t hash_value(const expression & expr)
    {
        return expr.hash_value();
    }

    future<expression *> simplification_loop(analysis_context & ctx, std::unique_ptr<expression> & uptr);

    template<typename... Ts>
    std::vector<std::unique_ptr<expression>> unique_expr_list(Ts &&... ts)
    {
        std::vector<std::unique_ptr<expression>> ret;
        ret.reserve(sizeof...(ts));
        (ret.emplace_back(std::forward<Ts>(ts)), ...);
        return ret;
    }
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct expression;
}
}
namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;
    std::unique_ptr<expression> preanalyze_expression(precontext & ctx,
        const parser::expression & expr,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name = std::nullopt);
}
}
