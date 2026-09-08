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

#include <absl/strings/str_cat.h>
#include <slang/syntax/SyntaxTree.h>
#include <slang/syntax/SyntaxVisitor.h>
#include <slang/text/SourceManager.h>

#include <unordered_map>

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

std::string_view getHdlname(const HierarchyInstantiationSyntax& syntax) {
    for (const auto* attribute : syntax.attributes) {
        for (const auto* spec : attribute->specs) {
            if (spec->name.valueText() == "hdlname" && spec->value &&
                spec->value->expr->kind == SyntaxKind::StringLiteralExpression) {
                return spec->value->expr->as<LiteralExpressionSyntax>().literal.valueText();
            }
        }
    }
    return {};
}

};  // namespace

std::vector<Module> SlangModuleCollector::collect(const std::shared_ptr<SyntaxTree>& tree) const {
    std::vector<const ModuleDeclarationSyntax*> declarations;
    std::unordered_map<std::string, unsigned int> existing_modules;
    std::vector<Module> modules;

    tree->root().visit(makeSyntaxVisitor(
        [&](auto& visitor, const ModuleDeclarationSyntax& declaration) {
            const auto name = std::string(declaration.header->name.valueText());
            existing_modules.emplace(name, modules.size());
            modules.emplace_back(name);
            declarations.push_back(&declaration);
            visitor.visitDefault(declaration);
        },
        [&](auto&, const IntegerTypeSyntax& type) {
            if (type.keyword.kind == parsing::TokenKind::RegKeyword) {
                LOG(WARNING) << "Only post-synthesis verilator is supported. 'reg' found at "
                             << printLoc(*tree, type.keyword.location());
            }
        }
    ));

    for (const auto* declaration : declarations) {
        const std::string name = std::string(declaration->header->name.valueText());
        Module& mod = modules[existing_modules.at(name)];
        declaration->visit(makeSyntaxVisitor([&](auto&,
                                                 const HierarchyInstantiationSyntax& syntax) {
            const std::string_view type = syntax.type.valueText();
            const std::string_view hdlname = getHdlname(syntax);
            for (const auto* instance : syntax.instances) {
                if (!instance->decl) {
                    const auto location = instance->sourceRange().start();
                    LOG(ERROR) << "Null instance declaration of " << type << " at "
                               << printLoc(*tree, location);
                    continue;
                }
                Cell cell{
                    .name = std::string(instance->decl->name.valueText()),
                    .type = std::string(type),
                    .hdlname = std::string(hdlname),
                    .width = 1,
                };
                if (const auto it = existing_modules.find(cell.type);
                    it != existing_modules.end()) {
                    VLOG(2) << "Cell's '" << cell.name << "' is child of module '" << it->first
                            << "'";
                    mod.child_modules.emplace_back(cell.name, it->second);
                }
                if (IsFlipFlop::check(cell, liberty)) {
                    VLOG(1) << "Cell '" << cell.name << "' is a flip-flop";
                    mod.cells.emplace_back(std::move(cell));
                } else {
                    VLOG(1) << "Cell '" << cell.name << "' is not a flip-flop. Skipping";
                }
            }
        }));
    }

    SEE_CHECK(!modules.empty()) << "No modules found";
    return modules;
}

std::vector<Module> SlangModuleCollector::collectFromFile(const std::filesystem::path& verilog
) const {
    SourceManager sourceManager;
    auto tree = slang::syntax::SyntaxTree::fromFile(verilog.c_str(), sourceManager);
    SEE_CHECK(tree) << "Failed to read " << verilog << ": " << tree.error().second;

    return collect(*tree);
}
