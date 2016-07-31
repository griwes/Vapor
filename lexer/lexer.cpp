/**
 * Vapor Compiler Licence
 *
 * Copyright © 2014-2015 Michał "Griwes" Dominiak
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

#include <reaver/unit.h>

#include "vapor/lexer.h"

using reaver::vapor::lexer::token_types;
using reaver::vapor::lexer::token_type;

std::array<std::string, +token_type::count> reaver::vapor::lexer::_v1::token_types;

static auto token_types_init = []() -> reaver::unit
{
    token_types[+token_type::identifier] = "identifier";
    token_types[+token_type::string] = "string";
    token_types[+token_type::integer] = "integer";
    token_types[+token_type::integer_suffix] = "integer-suffix";
    token_types[+token_type::module] = "module";
    token_types[+token_type::import] = "import";
    token_types[+token_type::auto_] = "auto";
    token_types[+token_type::let] = "let";
    token_types[+token_type::return_] = "return";
    token_types[+token_type::function] = "function";
    token_types[+token_type::dot] = ".";
    token_types[+token_type::curly_bracket_open] = "{";
    token_types[+token_type::curly_bracket_close] = "}";
    token_types[+token_type::square_bracket_open] = "[";
    token_types[+token_type::square_bracket_close] = "]";
    token_types[+token_type::round_bracket_open] = "(";
    token_types[+token_type::round_bracket_close] = ")";
    token_types[+token_type::angle_bracket_open] = "<";
    token_types[+token_type::angle_bracket_close] = ">";
    token_types[+token_type::colon] = ":";
    token_types[+token_type::semicolon] = ";";
    token_types[+token_type::map] = "->>";
    token_types[+token_type::bind] = "=>>";
    token_types[+token_type::assign] = "=";
    token_types[+token_type::block_value] = "=>";

    token_types[+token_type::logical_not] = "!";
    token_types[+token_type::bitwise_not] = "~";
    token_types[+token_type::bitwise_not_assignment] = "~=";

    token_types[+token_type::plus] = "+";
    token_types[+token_type::minus] = "-";
    token_types[+token_type::star] = "*";
    token_types[+token_type::slash] = "/";
    token_types[+token_type::modulo] = "%";
    token_types[+token_type::bitwise_and] = "&";
    token_types[+token_type::bitwise_or] = "|";
    token_types[+token_type::bitwise_xor] = "^";
    token_types[+token_type::logical_and] = "&&";
    token_types[+token_type::logical_or] = "||";
    token_types[+token_type::right_shift] = ">>";
    token_types[+token_type::left_shift] = "<<";

    token_types[+token_type::plus_assignment] = "+=";
    token_types[+token_type::minus_assignment] = "-=";
    token_types[+token_type::star_assignment] = "*=";
    token_types[+token_type::slash_assignment] = "/=";
    token_types[+token_type::modulo_assignment] = "%=";
    token_types[+token_type::bitwise_and_assignment] = "&=";
    token_types[+token_type::bitwise_or_assignment] = "|=";
    token_types[+token_type::bitwise_xor_assignment] = "^=";
    token_types[+token_type::logical_and_assignment] = "&&=";
    token_types[+token_type::logical_or_assignment] = "||=";
    token_types[+token_type::right_shift_assignment] = ">>=";
    token_types[+token_type::left_shift_assignment] = "<<=";

    token_types[+token_type::equals] = "==";
    token_types[+token_type::not_equals] = "!=";
    token_types[+token_type::less_equal] = "<=";
    token_types[+token_type::greater_equal] = ">=";

    token_types[+token_type::increment] = "++";
    token_types[+token_type::decrement] = "--";

    token_types[+token_type::lambda] = u8"λ";

    token_types[+token_type::none] = "<EMPTY TOKEN>";

    return {};
}();

const std::unordered_map<std::u32string, token_type> reaver::vapor::lexer::_v1::keywords = {
    { U"module", token_type::module },
    { U"import", token_type::import },
    { U"auto", token_type::auto_ },
    { U"let", token_type::let },
    { U"return", token_type::return_ },
    { U"function", token_type::function },
};

const std::unordered_map<char32_t, token_type> reaver::vapor::lexer::_v1::symbols1 = {
    { '.', token_type::dot },
    { ',', token_type::comma },
    { '{', token_type::curly_bracket_open },
    { '}', token_type::curly_bracket_close },
    { '[', token_type::square_bracket_open },
    { ']', token_type::square_bracket_close },
    { '(', token_type::round_bracket_open },
    { ')', token_type::round_bracket_close },
    { '<', token_type::angle_bracket_open },
    { '>', token_type::angle_bracket_close },
    { ':', token_type::colon },
    { ';', token_type::semicolon },
    { '=', token_type::assign },

    { '!', token_type::logical_not },
    { '~', token_type::bitwise_not },

    { '+', token_type::plus },
    { '-', token_type::minus },
    { '*', token_type::star },
    { '/', token_type::slash },
    { '%', token_type::modulo },
    { '&', token_type::bitwise_and },
    { '|', token_type::bitwise_or },
    { '^', token_type::bitwise_xor },
    { U'λ', token_type::lambda },
};

const std::unordered_map<char32_t, std::unordered_map<char32_t, token_type>> reaver::vapor::lexer::_v1::symbols2 = {
    { '<', {
        { '<', token_type::left_shift },
        { '=', token_type::less_equal }
    } },

    { '>', {
        { '>', token_type::right_shift },
        { '=', token_type::greater_equal }
    } },

    { '=', {
        { '>', token_type::block_value },
        { '=', token_type::equals }
    } },

    { '&', {
        { '&', token_type::logical_and },
        { '=', token_type::bitwise_and_assignment }
    } },

    { '|', {
        { '|', token_type::logical_or },
        { '=', token_type::bitwise_and_assignment }
    } },

    { '!', {
        { '=', token_type::not_equals }
    } },

    { '~', {
        { '=', token_type::bitwise_not_assignment }
    } },

    { '+', {
        { '=', token_type::plus_assignment }
    } },

    { '-', {
        { '=', token_type::minus_assignment }
    } },

    { '*', {
        { '=', token_type::star_assignment }
    } },

    { '/', {
        { '=', token_type::slash_assignment }
    } },

    { '%', {
        { '=', token_type::modulo_assignment }
    } },

    { '&', {
        { '=', token_type::bitwise_and_assignment }
    } },

    { '|', {
        { '=', token_type::bitwise_or_assignment }
    } },

    { '^', {
        { '=', token_type::bitwise_xor_assignment }
    } }
};

const std::unordered_map<char32_t, std::unordered_map<char32_t, std::unordered_map<char32_t, token_type>>> reaver::vapor::lexer::_v1::symbols3 = {
    { '-', {
        { '>', {
            { '>', token_type::map }
        } }
    } },
    { '=', {
        { '>', {
            { '>', token_type::bind }
        } }
    } },
};
