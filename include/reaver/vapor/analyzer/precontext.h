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

#pragma once

#include <stack>

#include <boost/filesystem.hpp>

#include "../config/compiler_options.h"
#include "expressions/entity.h"
#include "semantic/context.h"

namespace reaver::vapor::proto
{
class ast;
class user_defined_reference;
}

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    class unresolved_type;
    class overload_set;

    struct synthesized_udr
    {
        std::string name;
    };

    struct udr_compare
    {
        bool operator()(const proto::user_defined_reference * lhs,
            const proto::user_defined_reference * rhs) const;
        bool operator()(const synthesized_udr & lhs, const synthesized_udr & rhs) const;
    };

    struct udr_hash
    {
        std::size_t operator()(const proto::user_defined_reference * obj) const;
        std::size_t operator()(const synthesized_udr & obj) const;
    };

    struct precontext
    {
        const config::compiler_options & options;
        analysis_context & proper;
        std::unordered_map<std::string, std::unique_ptr<entity>> modules = {};

        std::unordered_map<synthesized_udr, std::unique_ptr<expression>, udr_hash, udr_compare>
            imported_entities = {};

        std::unordered_map<synthesized_udr, std::shared_ptr<overload_set>, udr_hash, udr_compare>
            imported_overload_sets = {};

        // shared to avoid needing a definition
        std::vector<std::unique_ptr<statement>> imported_default_instances = {};

        std::unordered_map<const proto::user_defined_reference *,
            std::shared_ptr<unresolved_type>,
            udr_hash,
            udr_compare>
            user_defined_types = {};

        scope * global_scope = nullptr;
        scope * current_lex_scope = nullptr;

        struct module_paths
        {
            boost::filesystem::path module_file_path;
            std::string_view source;
        };

        std::vector<module_paths> module_path_stack = {};
        std::vector<std::string> module_stack = {};
        std::string current_symbol = {};

        std::vector<std::shared_ptr<proto::ast>> imported_asts = {};

        std::size_t default_instance_definition_count = {};
    };
}
}
