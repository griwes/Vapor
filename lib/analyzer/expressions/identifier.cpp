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

#include "vapor/analyzer/expressions/identifier.h"
#include "vapor/parser.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    void identifier::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "identifier";
        print_address_range(os, this);
        os << ' ' << styles::string_value << utf8(_referenced_name) << '\n';

        auto expr_ctx = ctx.make_branch(!try_get_type());
        os << styles::def << expr_ctx << styles::subrule_name << "referenced expression";
        os << styles::def << " @ " << styles::address << _referenced << '\n';

        if (try_get_type())
        {
            auto type_ctx = ctx.make_branch(true);
            os << styles::def << type_ctx << styles::subrule_name << "referenced expression type:\n";
            get_type()->print(os, type_ctx.make_branch(true));
        }
    }

    future<> identifier::_analyze(analysis_context & ctx)
    {
        return get_scope()
            ->resolve(_referenced_name)
            ->get_expression_future()
            .then([&, this](auto && expression) {
                _referenced = expression;
                return _referenced->analyze(ctx);
            })
            .then([this] { this->_set_type(_referenced->get_type()); });
    }
}
}
