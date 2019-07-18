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

#include "vapor/analyzer/semantic/context.h"
#include "vapor/analyzer/expressions/expression.h"
#include "vapor/analyzer/expressions/runtime_value.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/semantic/typeclass_instance.h"
#include "vapor/analyzer/statements/default_instance.h"
#include "vapor/analyzer/statements/statement.h"
#include "vapor/analyzer/types/function.h"
#include "vapor/analyzer/types/sized_integer.h"
#include "vapor/analyzer/types/typeclass.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    std::size_t expression_list_hash::operator()(const std::vector<expression *> & arg_list) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, arg_list.size());
        for (auto && arg : arg_list)
        {
            boost::hash_combine(seed, arg->hash_value());
        }

        return seed;
    }

    bool expression_list_compare::operator()(const std::vector<expression *> & lhs,
        const std::vector<expression *> & rhs) const
    {
        return lhs.size() == rhs.size()
            && std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](auto && lhs, auto && rhs) {
                   return lhs->is_equal(rhs);
               });
    }

    std::size_t type_list_hash::operator()(const std::vector<type *> & arg_list) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, arg_list.size());
        for (auto && arg : arg_list)
        {
            boost::hash_combine(seed, arg);
        }

        return seed;
    }

    bool type_list_compare::operator()(const std::vector<type *> & lhs, const std::vector<type *> & rhs) const
    {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    analysis_context::analysis_context(future_promise_pair<void> default_instance_setup)
        : results{ std::make_shared<cached_results>() },
          simplification_ctx{ std::make_shared<simplification_context>(*results) },
          _default_instances_future{ std::move(default_instance_setup.future) },
          _default_instances_promise{ std::move(default_instance_setup.promise) }
    {
    }

    analysis_context::analysis_context() : analysis_context{ make_promise<void>() }
    {
    }

    sized_integer * analysis_context::get_sized_integer_type(std::size_t size)
    {
        auto & ret = _sized_integers[size];
        if (!ret)
        {
            ret = make_sized_integer_type(size);
        }

        return ret.get();
    }

    function_type * analysis_context::get_function_type(function_signature sig)
    {
        auto & ret = _function_types[sig];
        if (!ret)
        {
            ret = std::make_unique<function_type>(sig.return_type, std::move(sig.parameters));
        }

        return ret.get();
    }

    typeclass_type * analysis_context::get_typeclass_type(std::vector<type *> param_types)
    {
        auto & ret = _typeclass_types[param_types];
        if (!ret)
        {
            ret = std::make_unique<typeclass_type>(std::move(param_types));
        }

        return ret.get();
    }

    future<> analysis_context::default_instances_future() const
    {
        return _default_instances_future;
    }

    void analysis_context::set_default_instance_definition_count(std::size_t count)
    {
        assert(_unprocessed_default_instance_definitions == 0);
        _unprocessed_default_instance_definitions = count;

        if (count == 0)
        {
            _default_instances_promise.set();
        }
    }

    void analysis_context::add_default_instance_definition(default_instance * inst)
    {
        // instance transformed function
        auto itf = make_function("default instance selector");
        itf->set_parameters(fmap(inst->get_defined_instance()->get_argument_values(),
            [&](type * arg) { return make_runtime_value(arg); }));

        _typeclass_default_instances[inst->get_defined_instance()->get_typeclass()].push_back(std::move(itf));

        if (--_unprocessed_default_instance_definitions == 0)
        {
            _default_instances_promise.set();
        }
    }
}
}
