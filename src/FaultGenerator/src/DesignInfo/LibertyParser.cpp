// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "LibertyParser.h"

#include "LogUtils.h"
#include "Utils.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <istream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

enum class TokenKind {
    Eof,
    Value,
    LParen,
    RParen,
    LBrace,
    RBrace,
    Colon,
    Semicolon,
    Comma,
    DummyToken
};

struct Token {
    TokenKind kind;
    std::string_view str;

    bool is(TokenKind k) const { return kind == k; }
};

struct Lexer {
    // TODO fill this class with LOGs
    const char* cur;
    const char* end;
    int bracket_depth = 0;

    Lexer(std::string_view src) : cur(src.data()), end(src.data() + src.size()) {}

    /** Does the buffer contain at least `n` more characters */
    bool at_least(int n) const { return cur + n <= end; }

    /** Does the buffer starts with any character from the set */
    bool starts_with(CharSet set) const {
        assert(at_least(1));
        return set.contains(*cur);
    }

    /** Does the buffer starts with whitespace */
    bool starts_with_whitespace() const {
        assert(at_least(1));
        return *cur <= ' ';
    }

    /** Does the buffer's second character is a character from the set */
    bool peek(CharSet set) const { return at_least(2) && set.contains(cur[1]); }

    Token scan() {
        using enum TokenKind;

        // Skip whitespace, backslashes, and comments
        while (at_least(1)) {
            if (starts_with_whitespace() || starts_with("\\")) {
                ++cur;
            } else if (starts_with("/") && peek("*")) {
                cur += 2;
                while (at_least(1) && !(starts_with("*") && peek("/"))) {
                    ++cur;
                }
                if (at_least(1)) {
                    cur += 2;
                }
            } else if (starts_with("/") && peek("/")) {
                while (at_least(1) && !starts_with("\n")) {
                    ++cur;
                }
            } else {
                break;
            }
        }
        if (!at_least(1)) {
            return {Eof, {}};
        }

        // Delimiters
        switch (*cur) {
            case '(':
                return {LParen, {cur++, 1}};
            case ')':
                return {RParen, {cur++, 1}};
            case '{':
                return {LBrace, {cur++, 1}};
            case '}':
                return {RBrace, {cur++, 1}};
            case ':':
                return {Colon, {cur++, 1}};
            case ';':
                return {Semicolon, {cur++, 1}};
            case ',':
                return {Comma, {cur++, 1}};
        }

        // TODO: Distinguish strings, numbers, etc as token kinds
        // Quoted values (strings)
        if (at_least(1) && starts_with("\"")) {
            auto start = ++cur;
            while (at_least(1) && !starts_with("\"")) {
                if (starts_with("\\") && at_least(2)) {
                    ++cur;
                }
                ++cur;
            }
            auto end = cur;
            if (at_least(1)) {
                ++cur;
            }
            return {Value, {start, std::size_t(end - start)}};
        }

        // Unquoted values (until delimiter or whitespace)
        auto start = cur;
        while (at_least(1) && !starts_with_whitespace() && !starts_with(";,(){}:")) {
            ++cur;
        }
        return {Value, {start, std::size_t(cur - start)}};
    }
};

struct Item {
    std::string_view name;
    std::vector<std::string_view> args;  // TODO: Usually just a few args, do a small-vec
                                         // optimization (or even just bound it)
};

struct Iterator;

struct Parser {
    // TODO fill this class with LOGs
    Lexer* lexer;
    int depth;
    Token cur;

    Parser(Lexer* l, int d, Token c) : lexer(l), depth(d), cur(c) {}
    Parser(Lexer* l) : lexer(l), depth(0), cur(l->scan()) {}

    void next_token() { cur = lexer->scan(); }

    bool expect(TokenKind k) {
        static const char* names[] = {
            "end of file", "value", "'('", "')'", "'{'", "'}'", "':'", "';'", "','"
        };
        bool ok = cur.is(k);
        if (!ok) {
            LOG(WARNING) << "expected " << names[(int)k] << ", got '" << cur.str << "'";
        }
        next_token();
        return ok;
    }

    Parser enter() {
        if (lexer->bracket_depth <= depth) {
            return Parser(lexer, lexer->bracket_depth, {});
        }
        return Parser(lexer, lexer->bracket_depth, lexer->scan());
    }

    std::optional<Item> next() {
        using enum TokenKind;

        // Skip groups we didn't enter
        // TODO: Currently wastes time on tokenization, do a fast path that just looks at the braces
        while (lexer->bracket_depth > depth && !cur.is(Eof)) {
            if (cur.is(LBrace)) {
                ++lexer->bracket_depth;
            } else if (cur.is(RBrace)) {
                --lexer->bracket_depth;
            }
            next_token();
        }

        while (true) {
            if (cur.is(Eof)) {
                if (depth > 0) {
                    LOG(WARNING) << "unexpected end of file";
                }
                return {};
            }
            if (cur.is(RBrace)) {
                if (depth > 0) {
                    lexer->cur = cur.str.data();  // restore to '}' for parent's auto-skip
                    return {};
                }
                LOG(WARNING) << "unexpected '}'";
                next_token();
                continue;
            }

            auto name = cur.str;
            if (!expect(Value)) {
                continue;
            }

            // Simple attribute
            if (cur.is(Colon)) {
                next_token();
                std::string_view value;
                if (cur.is(Value)) {
                    value = cur.str;
                    next_token();
                }
                if (cur.is(Semicolon)) {
                    next_token();
                }
                return Item{name, {value}};
            }

            // Complex attribute or Group attribute
            if (cur.is(LParen)) {
                next_token();
                std::vector<std::string_view> args;
                while (cur.is(Value)) {
                    args.push_back(cur.str);
                    next_token();
                    if (!cur.is(RParen)) {
                        expect(Comma);
                    }
                }
                expect(RParen);
                if (cur.is(LBrace)) {
                    // Group attribute
                    ++lexer->bracket_depth;
                    cur = {TokenKind::DummyToken, {}};
                    return Item{name, std::move(args)};
                } else {
                    // Complex attribute
                    if (cur.is(Semicolon)) {
                        next_token();
                    }
                }
                return Item{name, std::move(args)};
            }
        }
    }

    Iterator begin();
    Iterator end();
};

struct Iterator {
    Parser* parser = nullptr;
    std::optional<std::pair<Item, Parser>> val;

    Iterator() = default;
    Iterator(Parser* p) : parser(p) { next(); }

    void next() {
        auto item = parser->next();
        if (item) {
            val.emplace(std::move(*item), parser->enter());
        } else {
            val.reset();
        }
    }

    std::pair<Item, Parser>& operator*() { return *val; }
    Iterator& operator++() {
        next();
        return *this;
    }
    bool operator!=(const Iterator& o) const { return val.has_value() != o.val.has_value(); }
};

inline Iterator Parser::begin() {
    return {this};
}
inline Iterator Parser::end() {
    return {};
}

/*****************************************************************************/

namespace {

bool isGzFile(std::ifstream& file) {
    auto file_begin_cursor = file.tellg();

    std::uint8_t b1, b2;
    file.read(reinterpret_cast<char*>(&b1), 1);
    file.read(reinterpret_cast<char*>(&b2), 1);

    file.seekg(file_begin_cursor);
    return b1 == 0x1f && b2 == 0x8b;
}

}  // namespace

std::optional<LibertyInfo> LibertyParser::parse(const std::string& filepath) {
    std::ifstream file{filepath};
    if (!file) {
        PLOG(ERROR) << "Error while opening '" << filepath << "'. Skipping file";
        return std::nullopt;
    }
    if (isGzFile(file)) {
        LOG(ERROR) << "Cannot parse a compressed file '" << filepath << "'. Skipping file";
        return std::nullopt;
    }

    std::string src{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    Lexer lexer(src);
    Parser parser(&lexer);
    LibertyInfo res;

    for (auto [lib_item, lib_parser] : parser) {
        res.name = lib_item.name;
        for (auto [cell_item, cell_parser] : lib_parser) {
            if (cell_item.name != "cell") {
                continue;
            }
            auto name = cell_item.args[0];
            std::optional<double> area;
            std::optional<FlipFlopInfo> ff_info;
            for (auto [field, _] : cell_parser) {
                if (field.name == "area") {
                    area = stodOpt(field.args[0]);
                    if (area) {
                        VLOG(2) << "Cell '" << name << "' has area: " << *area;
                    } else {
                        VLOG(2) << "Cell '" << name << "' has area: nullopt";
                    }
                } else if (field.name == "ff") {
                    ff_info = FlipFlopInfo{};
                    VLOG(2) << "Cell '" << name << "' is ff: " << *ff_info;
                }
            }
            try {
                res.cells.insert(
                    {std::string(name),
                     CellInfo{
                         .area = area.transform([this](double area) { return area * area_scale; }),
                         .ff_info = ff_info,
                     }}
                );
                VLOG(3) << "Successful insertion of cell: " << res.cells[std::string(name)];
            } catch (std::exception& e) {
                VLOG(2) << "Insertion failed: " << e.what();
            }
        }
    }

    return res;
}

std::vector<LibertyInfo> LibertyParser::parseFiles(const std::vector<std::string>& filepaths) {
    std::vector<LibertyInfo> res;
    res.reserve(filepaths.size());

    for (const auto& filepath : filepaths) {
        auto info_opt = parse(filepath);
        if (!info_opt) {
            continue;
        }
        res.push_back(std::move(*info_opt));
    }
    return res;
}
