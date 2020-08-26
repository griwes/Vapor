/**
 * Vapor Compiler Licence
 *
 * Copyright © 2015-2019 Michał "Griwes" Dominiak
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

#include <string>
#include <string_view>

#include <boost/locale/encoding_utf.hpp>

namespace reaver::vapor
{
inline namespace _v1
{
    inline auto utf8(const std::u32string & utf32)
    {
        return boost::locale::conv::utf_to_utf<char>(utf32);
    }

    inline auto utf8(const std::u32string_view & utf32)
    {
        return boost::locale::conv::utf_to_utf<char>(utf32.data());
    }

    inline auto utf32(const std::string & utf8)
    {
        return boost::locale::conv::utf_to_utf<char32_t>(utf8);
    }

    inline auto utf32(const std::string_view & utf8)
    {
        return boost::locale::conv::utf_to_utf<char32_t>(utf8.data());
    }
}
}
