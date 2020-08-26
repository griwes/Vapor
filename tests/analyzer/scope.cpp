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

#include <reaver/mayfly.h>

#include <reaver/future_get.h>

#include "helpers.h"

using namespace reaver::vapor;
using namespace reaver::vapor::analyzer;

MAYFLY_BEGIN_SUITE("analyzer");
MAYFLY_BEGIN_SUITE("scope");

MAYFLY_ADD_TESTCASE("init and get", [] {
    scope s{};

    auto present = make_symbol(U"present");
    auto present_ptr = present.get();

    MAYFLY_REQUIRE(s.init(U"present", std::move(present)));
    MAYFLY_CHECK(s.get(U"present") == present_ptr);
    auto tried = s.try_get(U"present");
    MAYFLY_CHECK(tried && tried.value() == present_ptr);
    MAYFLY_CHECK(s.get_or_init(U"present", []() -> std::unique_ptr<symbol> {
        MAYFLY_REQUIRE(!"the init function shouldn't be called!");
        __builtin_unreachable();
    }) == present_ptr);

    MAYFLY_REQUIRE(!s.init(U"present", make_symbol(U"present")));

    MAYFLY_CHECK_THROWS_TYPE(failed_lookup, s.get(U"absent"));
    MAYFLY_CHECK(!s.try_get(U"absent"));

    auto another = make_symbol(U"another");
    auto another_ptr = another.get();
    MAYFLY_REQUIRE(s.get_or_init(U"another", [&] { return std::move(another); }) == another_ptr);
});

MAYFLY_ADD_TESTCASE("resolve", [] {
    scope parent;
    auto child = parent.clone_for_type(U"child");

    auto parent_only = make_symbol(U"parent_only");
    auto parent_only_ptr = parent_only.get();
    parent.init(U"parent_only", std::move(parent_only));

    auto child_only = make_symbol(U"child_only");
    auto child_only_ptr = child_only.get();
    child->init(U"child_only", std::move(child_only));

    auto overloaded_parent = make_symbol(U"overloaded");
    parent.init(U"overloaded", std::move(overloaded_parent));

    auto overloaded_child = make_symbol(U"overloaded");
    auto overloaded_child_ptr = overloaded_child.get();
    child->init(U"overloaded", std::move(overloaded_child));

    auto parent_only_symbol = child->resolve(U"parent_only");
    auto child_only_symbol = child->resolve(U"child_only");
    auto overloaded_symbol = child->resolve(U"overloaded");
    MAYFLY_CHECK_THROWS_TYPE(failed_lookup, child->resolve(U"failed"));

    auto grandchild = child->clone_for_type(U"grandchild");

    auto grandchild_symbol = grandchild->resolve(U"parent_only");

    MAYFLY_CHECK(parent_only_symbol == parent_only_ptr);
    MAYFLY_CHECK(child_only_symbol == child_only_ptr);
    MAYFLY_CHECK(overloaded_symbol == overloaded_child_ptr);
    MAYFLY_CHECK(grandchild_symbol == parent_only_ptr);
});

MAYFLY_ADD_TESTCASE("is_local", [] {
    scope s1{}; // is_local = false

    auto s2 = s1.clone_for_decl();
    MAYFLY_CHECK(&s1 == s2);

    auto s3 = s1.clone_for_local(); // is_local = true
    std::unique_ptr<scope> s4{ s3->clone_for_decl() };
    MAYFLY_CHECK(s3.get() != s4.get());

    auto s5 = s1.clone_for_type(U"child"); // is_local = false
    auto s6 = s5->clone_for_decl();
    MAYFLY_CHECK(s5.get() == s6);
});

MAYFLY_ADD_TESTCASE("shadowing boundary", [] {
    scope s0;
    auto s1 = s0.clone_for_local(); // is_shadowing_boundary = true

    s1->init(U"symbol", make_symbol(U"symbol"));

    std::unique_ptr<scope> s2{ s1->clone_for_decl() }; // is_shadowing_boundary = false
    MAYFLY_CHECK(!s2->init(U"symbol", make_symbol(U"symbol")));

    auto s3 = s1->clone_for_local(); // is_shadowing_boundary = true
    MAYFLY_CHECK(s3->init(U"symbol", make_symbol(U"symbol")));

    auto s4 = s1->clone_for_type(U"s4"); // is_shadowing_boundary = true
    MAYFLY_CHECK(s4->init(U"symbol", make_symbol(U"symbol")));
});

MAYFLY_END_SUITE;
MAYFLY_END_SUITE;
