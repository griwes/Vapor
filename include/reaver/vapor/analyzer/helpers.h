/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014 Michał "Griwes" Dominiak
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

#include <boost/variant.hpp>

#include <reaver/error.h>
#include <reaver/id.h>
#include <reaver/tpl/vector.h>

namespace reaver
{
    namespace vapor
    {
        namespace analyzer { inline namespace _v1
        {
            inline error_engine & default_error_engine()
            {
                static error_engine engine;
                return engine;
            }

            template<typename Parse>
            void error(std::string message, const Parse & parse, error_engine & engine)
            {
                engine.push(exception(logger::error) << std::move(message));
            }

            template<typename Parse>
            void error(std::string message, const Parse & parse)
            {
                error(std::move(message), parse, default_error_engine());
            }
        }}
    }
}
