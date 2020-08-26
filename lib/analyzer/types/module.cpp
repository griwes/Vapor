/**
 * Vapor Compiler Licence
 *
 * Copyright © 2018-2020 Michał "Griwes" Dominiak
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

#include "vapor/analyzer/types/module.h"
#include "vapor/analyzer/expressions/entity.h"
#include "vapor/analyzer/semantic/symbol.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    void module_type::add_symbol(std::string name, expression * entity, bool is_visible)
    {
        auto u32name = utf32(name);
        if (u32name.substr(0, _member_scope->get_scoped_name().size()) == _member_scope->get_scoped_name())
        {
            u32name = u32name.substr(_member_scope->get_scoped_name().size());
        }

        auto symbol = make_symbol(u32name, entity);
        if (!is_visible)
        {
            symbol->hide();
        }
        _member_scope->init(u32name, std::move(symbol));
    }

    void module_type::close_scope()
    {
        _member_scope->close();
    }

    void module_type::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::type << "module type";
        os << styles::def << " @ " << styles::address << this;
        os << styles::def << ": " << styles::string_value << _name << '\n';
    }
}
}
