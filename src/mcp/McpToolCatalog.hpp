// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "McpTypes.hpp"
#include <vector>
#include <optional>
#include <memory>

namespace pdfinity::mcp {

struct McpToolResult {
    bool isError{false};
    std::string textContent;
    QJsonObject structuredData;

    static McpToolResult Success(const std::string& text, const QJsonObject& data = {}) {
        return {false, text, data};
    }

    static McpToolResult Error(const std::string& errorMsg) {
        return {true, errorMsg, {}};
    }
};

class McpToolCatalog {
public:
    static McpToolCatalog& Instance();

    const std::vector<McpToolDefinition>& GetAllTools() const;
    std::optional<McpToolDefinition> FindTool(const std::string& name) const;

    McpToolResult ExecuteTool(const std::string& name, const QJsonObject& arguments);

private:
    McpToolCatalog();
    void RegisterBuiltInTools();

    std::vector<McpToolDefinition> m_tools;
};

} // namespace pdfinity::mcp
