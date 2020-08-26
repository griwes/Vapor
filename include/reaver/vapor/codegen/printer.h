/**
 * Vapor Compiler Licence
 *
 * Copyright © 2017-2019 Michał "Griwes" Dominiak
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

#include <boost/algorithm/string/join.hpp>

#include <reaver/prelude/functor.h>

#include "generator.h"
#include "ir/variable.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    class ir_printer : public code_generator
    {
    public:
        virtual std::u32string generate_definitions(std::vector<ir::entity> &, codegen_context &) override;
        virtual std::u32string generate_definition(std::shared_ptr<ir::type>, codegen_context &) override;

        std::u32string generate_definition(const ir::variable &, codegen_context &);
        std::u32string generate_definition(const ir::function &, codegen_context &);
        std::u32string generate_definition(const ir::member_variable &, codegen_context &);

        std::u32string generate(const ir::instruction &, codegen_context &);

    private:
        template<typename T>
        static std::u32string _pointer_to_string(T * ptr)
        {
            std::stringstream ss;
            ss << ptr;
            return utf32(ss.str());
        }

        static std::u32string _to_string(const ir::value &);
    };

    inline std::shared_ptr<code_generator> make_printer()
    {
        return std::make_shared<ir_printer>();
    }
}
}
