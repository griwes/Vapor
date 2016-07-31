/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2015 Michał "Griwes" Dominiak
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

#include "vapor/utf8.h"
#include "vapor/lexer.h"
#include "vapor/parser.h"
#include "vapor/analyzer.h"

std::u32string program = UR"program(module hello_world
{
    let foo = λ => 1 * 2 + 3 * 4;

    function entry()
    {
        return foo();
    }
})program";

int main() try
{
    reaver::logger::dlog() << "Input:";
    reaver::logger::dlog() << reaver::vapor::utf8(program);
    reaver::logger::dlog();

    reaver::logger::dlog() << "Tokens:";
    reaver::vapor::lexer::iterator iterator{ program.begin(), program.end() };
    for (auto it = iterator; it; ++it)
    {
        reaver::logger::dlog() << *it;
    }
    reaver::logger::dlog();

    reaver::logger::dlog() << "AST:";
    reaver::vapor::parser::ast ast{ iterator };
    reaver::logger::dlog() << ast;

    reaver::logger::dlog() << "Analyzed AST:";
    reaver::vapor::analyzer::ast analyzed_ast{ ast };
//    reaver::logger::dlog() << analyzed_ast;
}

catch (reaver::exception & e)
{
    e.print(reaver::logger::default_logger());

    if (e.level() >= reaver::logger::crash)
    {
        return 2;
    }

    return 1;
}

catch (std::exception & e)
{
    reaver::logger::dlog(reaver::logger::crash) << e.what();

    return 2;
}

