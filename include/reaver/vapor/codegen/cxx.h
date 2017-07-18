/**
 * Vapor Compiler Licence
 *
 * Copyright © 2016-2017 Michał "Griwes" Dominiak
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

#include "generator.h"
#include "ir/variable.h"

namespace reaver::vapor::codegen
{
inline namespace _v1
{
    class cxx_generator : public code_generator
    {
    public:
        virtual std::u32string generate_declaration(ir::variable &, codegen_context &) const override;
        virtual std::u32string generate_declaration(ir::function &, codegen_context &) const override;
        virtual std::u32string generate_declaration(const std::shared_ptr<ir::variable_type> &, codegen_context &) const override;
        virtual std::u32string generate_definition(const ir::variable &, codegen_context &) override;
        virtual std::u32string generate_definition(const ir::function &, codegen_context &) override;
        virtual std::u32string generate_definition(const std::shared_ptr<ir::variable_type> &, codegen_context &) override;
        virtual std::u32string generate_definition(const ir::member_variable &, codegen_context &) override;

        virtual std::u32string generate(const ir::instruction &, codegen_context &) override;

        std::u32string get_storage_for(std::shared_ptr<ir::variable_type>, codegen_context &);
        void free_storage_for(std::u32string, std::shared_ptr<ir::variable_type>, codegen_context &);

        void clear_storage()
        {
            _unallocated_variables.clear();
        }

    private:
        mutable std::unordered_map<std::shared_ptr<ir::variable_type>, std::vector<std::u32string>> _unallocated_variables;
    };

    namespace cxx
    {
        template<typename T>
        std::u32string generate(const ir::instruction &, codegen_context &);

        std::u32string value_of(const ir::value &, codegen_context &, bool = false);
        std::u32string variable_of(const ir::value &, codegen_context &);
        void mark_destroyed(const ir::value &, codegen_context &);
    }

    inline std::shared_ptr<code_generator> make_cxx()
    {
        return std::make_shared<cxx_generator>();
    }
}
}
