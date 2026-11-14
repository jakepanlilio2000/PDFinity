// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SecureString.hpp"
#include <cstring>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace pdfinity::security {

namespace {
void SecureWipeMemory(void* ptr, size_t size) noexcept {
    if (!ptr || size == 0) return;
#if defined(_WIN32)
    SecureZeroMemory(ptr, size);
#elif defined(__STDC_LIB_EXT1__)
    memset_s(ptr, size, 0, size);
#else
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (size--) *p++ = 0;
#endif
}
} // namespace

SecureString::SecureString() = default;

SecureString::SecureString(const std::string& str) 
    : SecureString(str.data(), str.size()) {}

SecureString::SecureString(const char* str) {
    if (str) {
        size_t len = std::strlen(str);
        m_data.resize(len + 1);
        std::memcpy(m_data.data(), str, len);
        m_data[len] = '\0';
    }
}

SecureString::SecureString(const char* data, size_t length) {
    if (data && length > 0) {
        m_data.resize(length + 1);
        std::memcpy(m_data.data(), data, length);
        m_data[length] = '\0';
    }
}

SecureString::~SecureString() {
    wipe();
}

SecureString::SecureString(const SecureString& other) {
    if (!other.m_data.empty()) {
        m_data = other.m_data;
    }
}

SecureString& SecureString::operator=(const SecureString& other) {
    if (this != &other) {
        wipe();
        m_data = other.m_data;
    }
    return *this;
}

SecureString::SecureString(SecureString&& other) noexcept 
    : m_data(std::move(other.m_data)) {}

SecureString& SecureString::operator=(SecureString&& other) noexcept {
    if (this != &other) {
        wipe();
        m_data = std::move(other.m_data);
    }
    return *this;
}

const char* SecureString::c_str() const noexcept {
    return m_data.empty() ? "" : m_data.data();
}

std::span<const char> SecureString::as_span() const noexcept {
    if (m_data.empty()) return {};
    return { m_data.data(), m_data.size() - 1 };
}

void SecureString::clear() noexcept {
    wipe();
    m_data.clear();
}

void SecureString::wipe() noexcept {
    if (!m_data.empty()) {
        SecureWipeMemory(m_data.data(), m_data.size());
    }
}

} // namespace pdfinity::security
