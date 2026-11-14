// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <span>

namespace pdfinity::security {

class SecureString {
public:
    SecureString();
    explicit SecureString(const std::string& str);
    explicit SecureString(const char* str);
    SecureString(const char* data, size_t length);
    ~SecureString();

    SecureString(const SecureString& other);
    SecureString& operator=(const SecureString& other);

    SecureString(SecureString&& other) noexcept;
    SecureString& operator=(SecureString&& other) noexcept;

    [[nodiscard]] bool empty() const noexcept { return m_data.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_data.empty() ? 0 : m_data.size() - 1; }
    [[nodiscard]] const char* c_str() const noexcept;
    [[nodiscard]] std::span<const char> as_span() const noexcept;

    void clear() noexcept;

private:
    std::vector<char> m_data;
    void wipe() noexcept;
};

} // namespace pdfinity::security
