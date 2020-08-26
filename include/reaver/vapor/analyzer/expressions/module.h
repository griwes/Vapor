/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014, 2016-2020 Michał "Griwes" Dominiak
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

#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "../../range.h"
#include "../helpers.h"
#include "../ir_context.h"
#include "../semantic/function.h"
#include "../semantic/scope.h"
#include "../semantic/symbol.h"
#include "../statements/declaration.h"
#include "../statements/statement.h"
#include "../types/module.h"
#include "import.h"

namespace reaver::vapor::proto
{
struct module;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class module : public expression
    {
    public:
        module(ast_node parse,
            scope * lex_scope,
            std::u32string canonical_name,
            std::vector<std::u32string> name,
            module_type * typr,
            std::vector<std::unique_ptr<statement>> stmts);

        std::u32string name() const
        {
            return boost::join(_name, ".");
        }

        void print(std::ostream & os, print_context ctx) const override;

        auto get_ast_info() const
        {
            return std::make_optional(_parse);
        }

        future<> simplify_module(recursive_context ctx)
        {
            return simplify(ctx).then([](auto &&) {});
        }

        virtual declaration_ir declaration_codegen_ir(ir_generation_context &) const override
        {
            return {};
        }

        void generate_interface(proto::module &) const;

    private:
        virtual future<> _analyze(analysis_context &) override;

        virtual std::unique_ptr<expression> _clone_expr(replacements &) const override
        {
            assert(0);
        }

        virtual future<expression *> _simplify_expr(recursive_context) override;

        virtual statement_ir _codegen_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        virtual constant_init_ir _constinit_ir(ir_generation_context &) const override
        {
            assert(0);
        }

        ast_node _parse;
        module_type * _type = nullptr;
        std::vector<std::u32string> _name;
        std::vector<std::unique_ptr<statement>> _statements;
    };
}
}

namespace reaver::vapor::parser
{
inline namespace _v1
{
    struct module;
}
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    struct precontext;

    std::unique_ptr<module> preanalyze_module(precontext & ctx,
        const parser::module & parse,
        scope * lex_scope);
}
}
