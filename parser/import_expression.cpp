/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015 Michał "Griwes" Dominiak
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

#include "vapor/parser/import_expression.h"

reaver::vapor::parser::_v1::import_expression reaver::vapor::parser::_v1::parse_import_expression(reaver::vapor::parser::_v1::context & ctx)
{
    import_expression ret;

    auto start = expect(ctx, lexer::token_type::import).range.start();
    if (peek(ctx, lexer::token_type::string))
    {
        ret.module_name = parse_literal<lexer::token_type::string>(ctx);
    }
    else
    {
        ret.module_name = parse_id_expression(ctx);
    }
    fmap(ret.module_name, [&](const auto & elem) { ret.range = { start, elem.range.end() }; return unit{}; });

    return ret;
}

void reaver::vapor::parser::_v1::print(const reaver::vapor::parser::_v1::import_expression & expr, std::ostream & os, std::size_t indent)
{
    auto in = std::string(indent, ' ');

    os << in << "`import-expression` at " << expr.range << '\n';
    os << in << "{\n";
    fmap(expr.module_name, [&](const auto & elem) { print(elem, os, indent + 4); return unit{}; });
    os << in << "}\n";
}
