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

#pragma once

#include <memory>
#include <thread>
#include <atomic>

#include <reaver/semaphore.h>
#include <reaver/optional.h>

#include "vapor/position.h"
#include "vapor/lexer/token.h"
#include "vapor/lexer/detail/lexer_node.h"
#include "vapor/lexer/errors.h"

namespace reaver
{
    namespace vapor
    {
        namespace lexer { inline namespace _v1
        {
            namespace _detail
            {
                class _iterator_backend
                {
                public:
                    friend class lexer::iterator;

                    template<typename Iter>
                    _iterator_backend(Iter begin, Iter end, std::shared_ptr<_lexer_node> & node) : _thread{ [&](){ _worker(begin, end); } }
                    {
                        _sem.wait();
                        node = std::move(_initial);
                    }

                    ~_iterator_backend()
                    {
                        _end_flag = true;
                        _thread.join();
                    }

                private:
                    template<typename Iter>
                    void _worker(Iter begin, Iter end, std::string filename = "")
                    {
                        std::shared_ptr<_lexer_node> node = nullptr;

                        position pos;
                        pos.offset = -1;
                        pos.column = 0;
                        pos.line = 1;
                        pos.file = std::move(filename);

                        auto get = [&]() -> optional<char32_t>
                        {
                            if (begin == end)
                            {
                                return {};
                            }

                            if (*begin == U'\n')
                            {
                                pos.column = 0;
                                ++pos.line;
                            }
                            else
                            {
                                ++pos.column;
                            }

                            ++pos.offset;
                            return *begin++;
                        };

                        auto peek = [&](std::size_t x = 0) -> optional<char32_t>
                        {
                            for (std::size_t i = 0; i < x; ++i)
                            {
                                if (begin + i == end)
                                {
                                    return {};
                                }
                            }

                            return *(begin + x);
                        };

                        auto generate_token = [&](token_type type, position begin, position end, std::u32string string)
                        {
                            if (!node)
                            {
                                _initial = std::make_shared<_lexer_node>(token{ type, std::move(string), range_type(begin, end) }, _ex);
                                node = _initial;
                                _sem.notify();
                            }

                            else
                            {
                                node->_next = std::make_shared<_lexer_node>(token{ type, std::move(string), range_type(begin, end) }, _ex);
                                node->_sem.notify();
                                node = node->_next;
                            }
                        };

                        auto notify = [&]()
                        {
                            if (!node)
                            {
                                _sem.notify();
                                return;
                            }

                            node->_sem.notify();
                        };

                        auto is_white_space = [](char32_t c)
                        {
                            return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r';
                        };

                        auto is_identifier_start = [](char32_t c)
                        {
                            return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'_';
                        };

                        auto is_decimal = [&](char32_t c)
                        {
                            return c >= U'0' && c <= U'9';
                        };

                        auto is_identifier_char = [&](char32_t c)
                        {
                            return is_identifier_start(c) || is_decimal(c);
                        };

                        while (!_end_flag && begin != end)
                        {
                            auto next = get();

                            if (is_white_space(*next))
                            {
                                continue;
                            }

                            auto p = pos;
                            if (next == U'/')
                            {
                                auto second = peek();

                                if (second == U'/')
                                {
                                    while ((next = get()) && *next != U'\n')
                                    {
                                    }

                                    continue;
                                }

                                if (second == U'*')
                                {
                                    get();

                                    while ((next = get()) && (second = peek()) && next != U'*' && second != U'/')
                                    {
                                    }

                                    if (next && second && next == U'*' && second == U'/')
                                    {
                                        get();
                                        continue;
                                    }

                                    _ex = std::make_exception_ptr(unterminated_comment{ { p, pos } });
                                    notify();
                                    return;
                                }
                            }

                            {
                                auto second = peek();
                                auto third = peek(1);

                                if (second && third && symbols3.find(*next) != symbols3.end() && symbols3.at(*next).find(*second) != symbols3.at(*next).end()
                                    && symbols3.at(*next).at(*second).find(*third) != symbols3.at(*next).at(*second).end())
                                {
                                    auto p = pos;
                                    generate_token(symbols3.at(*next).at(*second).at(*third), p, p + 3, { *next, *get(), *get() });
                                    continue;
                                }

                                else if (second && symbols2.find(*next) != symbols2.end() && symbols2.at(*next).find(*second) != symbols2.at(*next).end())
                                {
                                    auto p = pos;
                                    generate_token(symbols2.at(*next).at(*second), p, p + 2, { *next, *get() })   ;
                                    continue;
                                }

                                else if (symbols1.find(*next) != symbols1.end())
                                {
                                    auto p = pos;
                                    generate_token(symbols1.at(*next), p, p + 1, { *next });
                                    continue;
                                }
                            }

                            std::u32string variable_length;

                            if (next == U'"')
                            {
                                auto second = peek();

                                while (next && second && (second != U'"' || next == U'\\') && (second != U'\n' || next == U'\\'))
                                {
                                    variable_length.push_back(*next);

                                    next = get();
                                    second = peek();
                                }

                                if (!next || second == U'\n')
                                {
                                    _ex = std::make_exception_ptr(unterminated_string{ { p, p + variable_length.size() } });
                                    notify();
                                    return;
                                }

                                variable_length.push_back(*next);
                                variable_length.push_back(*get());

                                generate_token(token_type::string, p, p + variable_length.size(), variable_length);
                                continue;
                            }

                            if (is_identifier_start(*next))
                            {
                                do
                                {
                                    variable_length.push_back(*next);
                                } while (peek() && is_identifier_char(*peek()) && (next = get()));

                                if (keywords.find(variable_length) != keywords.end())
                                {
                                    generate_token(keywords.at(variable_length), p, p + variable_length.size(), variable_length);
                                    continue;
                                }

                                generate_token(token_type::identifier, p, p + variable_length.size(), variable_length);
                                continue;
                            }

                            if (is_decimal(*next))
                            {
                                do
                                {
                                    variable_length.push_back(*next);
                                } while (peek() && is_decimal(*peek()) && (next = get()));

                                generate_token(token_type::integer, p, p + variable_length.size(), variable_length);

                                if (next && is_identifier_start(*next))
                                {
                                    variable_length.clear();

                                    do
                                    {
                                        variable_length.push_back(*next);
                                    } while (peek() && is_identifier_char(*peek()) && (next = get()));

                                    generate_token(token_type::integer_suffix, p, p + variable_length.size(), variable_length);
                                }

                                continue;
                            }

                            _ex = std::make_exception_ptr(exception(logger::fatal) << "stray character in file: " << utf8({ *next }));
                            notify();
                            return;
                        }

                        if (node)
                        {
                            node->_done = true;
                            node->_sem.notify();
                        }

                        else
                        {
                            _sem.notify();
                        }
                    }

                    std::atomic<bool> _end_flag{ false };
                    std::mutex _m;
                    std::shared_ptr<_lexer_node> _initial = nullptr;
                    semaphore _sem;
                    std::thread _thread;
                    std::exception_ptr _ex = nullptr;
                };
            }
        }}
    }
}
