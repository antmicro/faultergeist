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

#include "SlangModuleCollector.h"

#include "Cell.h"
#include "IsFlipFlopPredicate.h"
#include "LogUtils.h"
#include "Module.h"
#include "Utils.h"
#include "Wire.h"

#include <absl/strings/str_cat.h>
#include <slang/syntax/SyntaxTree.h>
#include <slang/syntax/SyntaxVisitor.h>
#include <slang/text/SourceManager.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace slang;
using namespace slang::syntax;

namespace {

std::string printLoc(const SyntaxTree& tree, const SourceLocation location) {
    const auto& source_manager = tree.sourceManager();
    return absl::StrCat(
        source_manager.getFileName(location),
        ":",
        source_manager.getLineNumber(location),
        ":",
        source_manager.getColumnNumber(location)
    );
}

std::string_view getHdlname(const SyntaxList<AttributeInstanceSyntax>& attributes) {
    for (const auto* attribute : attributes) {
        for (const auto* spec : attribute->specs) {
            if (spec->name.valueText() == "hdlname" && spec->value &&
                spec->value->expr->kind == SyntaxKind::StringLiteralExpression) {
                return spec->value->expr->as<LiteralExpressionSyntax>().literal.valueText();
            }
        }
    }
    return {};
}

bool hasAttribute(
    const SyntaxList<AttributeInstanceSyntax>& attributes,
    std::string_view attribute_name
) {
    if (attribute_name.empty()) {
        return true;
    }
    for (const auto* attribute : attributes) {
        for (const auto* spec : attribute->specs) {
            if (spec->name.valueText() == attribute_name) {
                return true;
            }
        }
    }
    return false;
}

struct DimensionInfo {
    std::uint32_t width;
    std::uint32_t lower;
};

std::optional<DimensionInfo> getDimensionInfo(const VariableDimensionSyntax& dimension) {
    if (!dimension.specifier || dimension.specifier->kind != SyntaxKind::RangeDimensionSpecifier) {
        return std::nullopt;
    }
    const auto& selector = *dimension.specifier->as<RangeDimensionSpecifierSyntax>().selector;
    if (selector.kind != SyntaxKind::SimpleRangeSelect) {
        return std::nullopt;
    }
    const auto& range = selector.as<RangeSelectSyntax>();
    if (range.left->kind != SyntaxKind::IntegerLiteralExpression ||
        range.right->kind != SyntaxKind::IntegerLiteralExpression) {
        return std::nullopt;
    }
    const auto left = range.left->as<LiteralExpressionSyntax>().literal.intValue().as<int64_t>();
    const auto right = range.right->as<LiteralExpressionSyntax>().literal.intValue().as<int64_t>();
    if (!left || !right || *left < 0 || *right < 0) {
        return std::nullopt;
    }
    const auto upper = static_cast<std::uint64_t>(std::max(*left, *right));
    const auto lower = static_cast<std::uint64_t>(std::min(*left, *right));
    const auto width = upper - lower + 1;
    if (width > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return DimensionInfo{
        .width = static_cast<std::uint32_t>(width),
        .lower = static_cast<std::uint32_t>(lower),
    };
}

struct WireDimensions {
    std::uint32_t width = 1;
    std::uint32_t offset = 0;
};

std::optional<WireDimensions> getWireDimensions(
    const DataTypeSyntax& type,
    const DeclaratorSyntax& declarator
) {
    WireDimensions result;
    std::optional<std::uint32_t> single_dimension_offset;
    std::size_t dimension_count = 0;
    auto applyDimensions = [&](const SyntaxList<VariableDimensionSyntax>& dimensions) {
        for (const auto* dimension : dimensions) {
            const auto info = getDimensionInfo(*dimension);
            if (!info) {
                return false;
            }
            if (info->width == 0 ||
                result.width > std::numeric_limits<std::uint32_t>::max() / info->width) {
                return false;
            }
            result.width *= info->width;
            single_dimension_offset = info->lower;
            ++dimension_count;
        }
        return true;
    };
    switch (type.kind) {
        case SyntaxKind::BitType:
        case SyntaxKind::LogicType:
        case SyntaxKind::RegType:
        case SyntaxKind::ByteType:
        case SyntaxKind::ShortIntType:
        case SyntaxKind::IntType:
        case SyntaxKind::LongIntType:
        case SyntaxKind::IntegerType:
        case SyntaxKind::TimeType: {
            const auto& integer_type = type.as<IntegerTypeSyntax>();
            switch (integer_type.keyword.kind) {
                case parsing::TokenKind::ByteKeyword:
                    result.width = 8;
                    break;
                case parsing::TokenKind::ShortIntKeyword:
                    result.width = 16;
                    break;
                case parsing::TokenKind::IntKeyword:
                case parsing::TokenKind::IntegerKeyword:
                    result.width = 32;
                    break;
                case parsing::TokenKind::LongIntKeyword:
                case parsing::TokenKind::TimeKeyword:
                    result.width = 64;
                    break;
                default:
                    break;
            }
            if (!applyDimensions(integer_type.dimensions)) {
                return std::nullopt;
            }
            break;
        }
        case SyntaxKind::ImplicitType:
            if (!applyDimensions(type.as<ImplicitTypeSyntax>().dimensions)) {
                return std::nullopt;
            }
            break;
        default:
            return std::nullopt;
    }
    if (!applyDimensions(declarator.dimensions)) {
        return std::nullopt;
    }
    // Yosys flattens multiple dimensions to a zero-based vector. Preserve a non-zero lower bound
    // only for the single-dimensional case, such as wire [31:16] counter.
    if (dimension_count == 1) {
        result.offset = *single_dimension_offset;
    }
    return result;
}

};  // namespace

std::vector<Module> SlangModuleCollector::collect(const std::shared_ptr<SyntaxTree>& tree) const {
    std::vector<const ModuleDeclarationSyntax*> declarations;
    std::unordered_map<std::string, unsigned int> existing_modules;
    std::vector<Module> modules;

    tree->root().visit(makeSyntaxVisitor([&](auto& visitor,
                                             const ModuleDeclarationSyntax& declaration) {
        const auto name = std::string(declaration.header->name.valueText());
        existing_modules.emplace(name, modules.size());
        modules.emplace_back(name);
        declarations.push_back(&declaration);
        visitor.visitDefault(declaration);
    }));

    for (const auto* declaration : declarations) {
        Module& mod =
            modules[existing_modules.at(std::string(declaration->header->name.valueText()))];
        const auto existing_cells = collectCells(*declaration, *tree, existing_modules, mod);
        if (collect_wires) {
            collectWires(*declaration, existing_cells, mod);
        }
    }

    SEE_CHECK(!modules.empty()) << "No modules found";
    return modules;
}

std::unordered_set<std::string> SlangModuleCollector::collectCells(
    const ModuleDeclarationSyntax& declaration,
    const SyntaxTree& tree,
    const std::unordered_map<std::string, unsigned int>& existing_modules,
    Module& mod
) const {
    std::unordered_set<std::string> existing_cells;
    declaration.visit(makeSyntaxVisitor([&](auto&, const HierarchyInstantiationSyntax& syntax) {
        const std::string_view type = syntax.type.valueText();
        const std::string_view hdlname = getHdlname(syntax.attributes);
        for (const auto* instance : syntax.instances) {
            if (!instance->decl) {
                const auto location = instance->sourceRange().start();
                LOG(ERROR) << "Null instance declaration of " << type << " at "
                           << printLoc(tree, location);
                continue;
            }
            Cell cell{
                .name = std::string(instance->decl->name.valueText()),
                .type = std::string(type),
                .hdlname = std::string(hdlname),
                .width = 1,
            };
            if (const auto it = existing_modules.find(cell.type); it != existing_modules.end()) {
                VLOG(2) << "Cell's '" << cell.name << "' is child of module '" << it->first << "'";
                mod.child_modules.emplace_back(cell.name, it->second);
            }
            if (IsFlipFlop::check(cell, liberty)) {
                VLOG(1) << "Cell '" << cell.name << "' is a flip-flop";
                existing_cells.emplace(stripSignalNameFromCellType(cell.name));
                mod.cells.emplace_back(std::move(cell));
            } else {
                VLOG(1) << "Cell '" << cell.name << "' is not a flip-flop. Skipping";
            }
        }
    }));
    return existing_cells;
}

void SlangModuleCollector::collectWires(
    const ModuleDeclarationSyntax& declaration,
    const std::unordered_set<std::string>& existing_cells,
    Module& mod
) const {
    declaration.visit(makeSyntaxVisitor(
        // Example of `PortDeclarationSyntax`.
        // module example(request, ready);
        //     input  [7:0] request;
        //     output       ready;
        // endmodule
        [&](auto&, const PortDeclarationSyntax& syntax) {
            for (const auto* declarator : syntax.declarators) {
                mod.ports.emplace(declarator->name.valueText());
            }
        },
        // Example of `ImplicitAnsiPortSyntax`.
        // module example(input [7:0] request, output ready);
        //     ...
        // endmodule
        [&](auto&, const ImplicitAnsiPortSyntax& syntax) {
            mod.ports.emplace(syntax.declarator->name.valueText());
        }
    ));
    std::vector<Wire> declared_wires;
    auto add_wire = [&](const DeclaratorSyntax& declarator,
                        const DataTypeSyntax& type,
                        std::string_view hdlname,
                        bool has_required_attribute) {
        const std::string wire_name(declarator.name.valueText());
        if (std::ranges::find(clk_names, findSignalName(wire_name)) != clk_names.end()) {
            VLOG(2) << "Skipping clock wire '" << wire_name << "'";
            return;
        }
        if (!has_required_attribute && !mod.ports.contains(wire_name)) {
            VLOG(2) << "Skipping wire '" << wire_name << "' without attribute '" << wire_attribute
                    << "'";
            return;
        }
        const auto dimensions = getWireDimensions(type, declarator);
        if (!dimensions) {
            LOG(WARNING) << "Skipping wire '" << wire_name << "' with unsupported dimensions";
            return;
        }
        const auto existing =
            std::find_if(declared_wires.begin(), declared_wires.end(), [&](const Wire& wire) {
                return wire.name == wire_name;
            });
        if (existing != declared_wires.end()) {
            existing->width = dimensions->width;
            existing->offset = dimensions->offset;
            if (!hdlname.empty()) {
                existing->hdlname = hdlname;
            }
            return;
        }
        declared_wires.push_back(Wire{
            .name = wire_name,
            .hdlname = std::string(hdlname),
            .width = dimensions->width,
            .offset = dimensions->offset,
            .is_port = mod.ports.contains(wire_name),
        });
    };
    declaration.visit(makeSyntaxVisitor(
        [&](auto&, const PortDeclarationSyntax& syntax) {
            const DataTypeSyntax* type = nullptr;
            if (syntax.header->kind == SyntaxKind::NetPortHeader) {
                type = syntax.header->as<NetPortHeaderSyntax>().dataType;
            } else if (syntax.header->kind == SyntaxKind::VariablePortHeader) {
                type = syntax.header->as<VariablePortHeaderSyntax>().dataType;
            }
            if (type) {
                const auto hdlname = getHdlname(syntax.attributes);
                const bool has_required_attribute = hasAttribute(syntax.attributes, wire_attribute);
                for (const auto* declarator : syntax.declarators) {
                    add_wire(*declarator, *type, hdlname, has_required_attribute);
                }
            }
        },
        [&](auto&, const NetDeclarationSyntax& syntax) {
            const auto hdlname = getHdlname(syntax.attributes);
            const bool has_required_attribute = hasAttribute(syntax.attributes, wire_attribute);
            for (const auto* declarator : syntax.declarators) {
                add_wire(*declarator, *syntax.type, hdlname, has_required_attribute);
            }
        },
        [&](auto&, const DataDeclarationSyntax& syntax) {
            const auto hdlname = getHdlname(syntax.attributes);
            const bool has_required_attribute = hasAttribute(syntax.attributes, wire_attribute);
            for (const auto* declarator : syntax.declarators) {
                add_wire(*declarator, *syntax.type, hdlname, has_required_attribute);
            }
        },
        [&](auto&, const ImplicitAnsiPortSyntax& syntax) {
            const DataTypeSyntax* type = nullptr;
            if (syntax.header->kind == SyntaxKind::NetPortHeader) {
                type = syntax.header->as<NetPortHeaderSyntax>().dataType;
            } else if (syntax.header->kind == SyntaxKind::VariablePortHeader) {
                type = syntax.header->as<VariablePortHeaderSyntax>().dataType;
            }
            if (type) {
                add_wire(
                    *syntax.declarator,
                    *type,
                    getHdlname(syntax.attributes),
                    hasAttribute(syntax.attributes, wire_attribute)
                );
            }
        }
    ));
    for (const Wire& wire : declared_wires) {
        auto non_register_bits =
            deduplicate_wires ? wire.removeFlipFlopBits(existing_cells) : wire.removeFlipFlopBits();
        mod.wires.insert(
            mod.wires.end(),
            std::make_move_iterator(non_register_bits.begin()),
            std::make_move_iterator(non_register_bits.end())
        );
    }
}

std::vector<Module> SlangModuleCollector::collectFromFile(const std::filesystem::path& verilog
) const {
    SourceManager sourceManager;
    auto tree = slang::syntax::SyntaxTree::fromFile(verilog.c_str(), sourceManager);
    SEE_CHECK(tree) << "Failed to read " << verilog << ": " << tree.error().second;

    return collect(*tree);
}
