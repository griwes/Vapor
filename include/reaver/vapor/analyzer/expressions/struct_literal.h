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

#include "../types/struct.h"
#include "expression.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class struct_literal : public expression
    {
    public:
        struct_literal(ast_node parse, std::unique_ptr<struct_type> type);

        virtual void print(std::ostream &, print_context) const override;

        virtual declaration_ir declaration_codegen_ir(ir_generation_context & ctx) const override
        {
            return _type->get_expression()->declaration_codegen_ir(ctx);
        }

        virtual void mark_exported() override;

    private:
        virtual expression * _get_replacement() override
        {
            return _type->get_expression();
        }

        virtual const expression * _get_replacement() const override
        {
            return _type->get_expression();
        }

        virtual future<> _analyze(analysis_context &) override;
        virtual std::unique_ptr<expression> _clone_expr(replacements & repl) const override;
        virtual future<expression *> _simplify_expr(recursive_context) override;
        virtual statement_ir _codegen_ir(ir_generation_context &) const override;
        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override;

        virtual bool _is_equal(const expression * rhs) const override
        {
            assert(0);
        }

        virtual std::unique_ptr<google::protobuf::Message> _generate_interface() const override;

        std::shared_ptr<struct_type> _type;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct struct_literal;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<struct_literal> preanalyze_struct_literal(precontext & ctx,
        const parser::struct_literal & parse,
        scope * lex_scope,
        std::optional<std::u32string> canonical_name = std::nullopt);
}
}
