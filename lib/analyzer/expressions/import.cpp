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

#include <numeric>

#include <boost/algorithm/string/join.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "vapor/analyzer/expressions/expression_ref.h"
#include "vapor/analyzer/expressions/import.h"
#include "vapor/analyzer/precontext.h"
#include "vapor/analyzer/semantic/symbol.h"
#include "vapor/analyzer/statements/default_instance.h"
#include "vapor/analyzer/types/module.h"
#include "vapor/analyzer/types/unresolved.h"
#include "vapor/parser/import_expression.h"
#include "vapor/sha.h"

#include "ast.pb.h"

namespace reaver::vapor::analyzer
{
inline namespace _v1
{
    template<typename Iter>
    std::optional<boost::filesystem::path> find_module(const boost::filesystem::path & module_path,
        Iter begin,
        Iter & end,
        bool source_only = false)
    {
        auto actual_path = std::accumulate(begin, end, module_path, std::divides<>());

        // inside a module directory that matches...
        if (boost::filesystem::is_directory(actual_path))
        {
            // if there is a compiled module and we are looking for more than just sources, return that
            if (!source_only && boost::filesystem::is_regular_file(actual_path / "module.vprm"))
            {
                return std::make_optional(actual_path / "module.vprm");
            }

            // else if there is a module source, return that
            if (boost::filesystem::is_regular_file(actual_path / "module.vpr"))
            {
                return std::make_optional(actual_path / "module.vpr");
            }

            // but if the directory exists, but there's no "module" file inside, throw an error
            // TODO: make this throw and not assert...
            assert(!"found a module directory, but no master module file inside...");
        }

        // if there is a non-directory compiled module that matches and we are interested in more than
        // sources...
        if (!source_only && boost::filesystem::is_regular_file(actual_path.replace_extension(".vprm")))
        {
            return std::make_optional(actual_path);
        }

        // else if there's a non-directory module source, return that
        if (boost::filesystem::is_regular_file(actual_path.replace_extension(".vpr")))
        {
            return std::make_optional(actual_path);
        }

        // otherwise, if there's a parent, try to find its parent somehow, maybe it defines the submodule
        if (begin != end - 1)
        {
            return find_module(module_path, begin, --end, source_only);
        }

        return std::nullopt;
    }

    std::optional<boost::filesystem::path> find_module(precontext & ctx,
        const std::vector<std::string> & module_name,
        bool source_only = false)
    {
        for (auto module_path : ctx.options.module_paths())
        {
            auto end = module_name.end();

            if (auto found_module = find_module(module_path, module_name.begin(), end, source_only))
            {
                return found_module;
            }
        }

        return std::nullopt;
    }

    entity * import_module(precontext & ctx, const std::vector<std::string> & module_name);

    void import_from_ast(precontext & ctx,
        const boost::filesystem::path & path,
        const std::vector<std::string> & module_name)
    {
        std::ifstream interface_file{ path.string() };
        if (!interface_file)
        {
            throw exception{ logger::fatal } << "couldn't open module interface file: " << path;
        }

        auto ast = std::make_shared<proto::ast>();
        ctx.imported_asts.push_back(ast);

        if (!ast->ParseFromIstream(&interface_file))
        {
            throw exception{ logger::fatal }
                << "couldn't parse the serialized ast from the module interface file " << path;
        }
        if (!ast->has_compilation_info() || ast->modules_size() == 0)
        {
            throw exception{ logger::fatal } << "no valid serialized ast in the module interface file "
                                             << path;
        }

        if (auto source_path = find_module(ctx, module_name, true))
        {
            auto check_module = [&ctx](auto && comp_time, auto && comp_hash, auto && module_name) {
                if (auto source_path = find_module(ctx, module_name, true))
                {
                    if (static_cast<std::int64_t>(boost::filesystem::last_write_time(source_path.value()))
                        > comp_time)
                    {
                        boost::iostreams::mapped_file_source source{ source_path->string() };
                        auto sha256sum = sha256(source.data(), source.size());

                        if (sha256sum != comp_hash)
                        {
                            return false;
                        }
                    }
                }

                return true;
            };

            bool is_up_to_date = check_module(
                ast->compilation_info().time(), ast->compilation_info().source_hash(), module_name);

            if (is_up_to_date)
            {
                for (auto && import : ast->imports())
                {
                    if (!check_module(import.target_compilation_time(),
                            import.target_source_hash(),
                            std::vector<std::string>{ import.name().begin(), import.name().end() }))
                    {
                        is_up_to_date = false;
                        break;
                    }
                }
            }

            if (!is_up_to_date)
            {
                ctx.options.compile_file(source_path.value());
                import_module(ctx, module_name);
                return;
            }
        }

        ctx.module_path_stack.push_back(
            { boost::filesystem::canonical(path), ast->compilation_info().filepath() });

        std::unordered_set<entity *> import_deps;
        if (ast->imports_size())
        {
            for (auto && import : ast->imports())
            {
                std::vector<std::string> name{ import.name().begin(), import.name().end() };
                import_deps.insert(import_module(ctx, name));
            }
        }

        for (auto && module : ast->modules())
        {
            auto name = boost::algorithm::join(module.name(), ".");
            ctx.module_stack.emplace_back(name);

            if (static_cast<std::size_t>(module.name_size()) < module_name.size()
                || std::mismatch(module_name.begin(), module_name.end(), module.name().begin()).first
                    != module_name.end())
            {
                throw exception{ logger::error } << "invalid module name `" << name << "` in module file "
                                                 << ctx.module_path_stack.back().module_file_path;
            }

            std::string cumulative_name;

            module_type * type = nullptr;
            scope * lex_scope = ctx.global_scope;

            // TODO: integrate this with the same stuff in preanalyze_module
            for (auto && name_part : module.name())
            {
                cumulative_name = cumulative_name + (cumulative_name.empty() ? "" : ".") + name_part;
                auto & saved = ctx.modules[cumulative_name];

                if (saved)
                {
                    assert(dynamic_cast<module_type *>(saved->get_type()));
                    lex_scope = saved->get_type()->get_scope();

                    continue;
                }

                auto scope = lex_scope->clone_for_module(utf32(name_part));
                auto old_scope = std::exchange(lex_scope, scope.get());

                auto type_uptr = std::make_unique<module_type>(std::move(scope), name_part);
                type = type_uptr.get();

                auto name = utf32(name_part);
                saved = make_entity(std::move(type_uptr), old_scope, name);
                auto symbol = make_symbol(std::move(name), saved.get());
                symbol->hide();
                saved->save_symbol(symbol.get());
                assert(old_scope->init(utf32(name_part), std::move(symbol)));

                saved->set_timestamp(ast->compilation_info().time());
                saved->set_source_hash(ast->compilation_info().source_hash());
                saved->set_import_name({ module.name().begin(), module.name().end() });
                for (auto && dep : import_deps)
                {
                    saved->add_import_dependency(dep);
                }
            }

            ctx.current_lex_scope = lex_scope;

            // seems that protobuf's map doesn't have a deterministic order of iteration
            // we can't let that happen in a compiler...
            std::vector<std::pair<const std::string *, const proto::entity *>> symbols;
            symbols.reserve(module.symbols().size());
            for (auto && entity : module.symbols())
            {
                symbols.emplace_back(&entity.first, &entity.second);
            }
            std::sort(symbols.begin(), symbols.end(), [](auto && lhs, auto && rhs) {
                return *lhs.first < *rhs.first;
            });

            for (auto && imported_entity : symbols)
            {
                ctx.current_symbol = *imported_entity.first;
                auto ent = get_entity(ctx, *imported_entity.second);
                type->add_symbol(
                    *imported_entity.first, ent.get(), imported_entity.second->is_name_exported());

                ctx.imported_entities.emplace(synthesized_udr{ ctx.current_symbol }, std::move(ent));
            }

            for (auto && default_instance : module.default_instances())
            {
                auto defn = import_default_instance(ctx, default_instance);
                ctx.imported_default_instances.push_back(std::move(defn));
            }

            ctx.module_stack.pop_back();
        }

        ctx.module_path_stack.pop_back();
    }

    entity * import_module(precontext & ctx, const std::vector<std::string> & module_name)
    {
        auto return_cached = [&]() -> entity * {
            auto name = boost::algorithm::join(module_name, ".");
            auto it = ctx.modules.find(name);
            if (it != ctx.modules.end())
            {
                return it->second.get();
            }

            return nullptr;
        };

        if (auto cached = return_cached())
        {
            return cached;
        }

        if (auto found_module = find_module(ctx, module_name))
        {
            if (found_module->extension() == ".vprm")
            {
                import_from_ast(ctx, found_module.value(), module_name);
                auto cached = return_cached();
                assert(cached);
                return cached;
            }

            if (found_module->extension() == ".vpr")
            {
                ctx.options.compile_file(found_module.value());
                return import_module(ctx, module_name);
            }

            assert(!"some weird unknown extension found by find_module!");
        }

        throw exception{ logger::error } << "couldn't find module `"
                                         << boost::algorithm::join(module_name, ".") << "`";
    }

    std::unique_ptr<import_expression> preanalyze_import(precontext & ctx,
        const parser::import_expression & parse,
        scope * lex_scope,
        std::optional<std::u32string> name,
        import_mode mode)
    {
        auto expr = std::get<0>(fmap(parse.module_name,
            make_overload_set(
                [&](const parser::id_expression & expr) {
                    auto module =
                        fmap(expr.id_expression_value, [](auto && id) { return utf8(id.value.string); });
                    auto ent = import_module(ctx, module);
                    assert(ent);

                    if (mode == import_mode::statement)
                    {
                        // FIXME: this is wrong, because parent symbols won't be unhidden
                        ent->get_symbol()->unhide();
                    }

                    return std::make_unique<import_expression>(
                        make_node(parse), ent, lex_scope, std::move(name));
                },
                [&](auto && expr) -> std::unique_ptr<import_expression> { assert(0); })));

        return expr;
    }

    void import_expression::print(std::ostream & os, print_context ctx) const
    {
        os << styles::def << ctx << styles::rule_name << "import-expression";
        print_address_range(os, this);
        os << ' ' << styles::string_value << boost::algorithm::join(_module->get_import_name(), ".") << '\n';

        auto module_ctx = ctx.make_branch(true);
        os << styles::def << module_ctx << styles::subrule_name << "referenced entity";
        os << styles::def << " @ " << styles::address << _module << '\n';

        // TODO: do I want to actually print the entity itself here?
        // I think I don't, but I'll need that as a debug tool one day
    }

    std::unique_ptr<expression> import_expression::_clone_expr(replacements &) const
    {
        return std::make_unique<import_expression>(get_ast_info().value(), _module, get_scope(), get_name());
    }

    future<expression *> import_expression::_simplify_expr(recursive_context ctx)
    {
        return make_ready_future<expression *>(this);
    }
}
}
