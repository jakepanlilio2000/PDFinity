// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PathSanitizer.hpp"
#include <algorithm>
#include <array>
#include <cctype>

namespace pdfinity::security {

namespace {
bool ContainsNullOrControlChars(const std::string& str) {
    for (unsigned char c : str) {
        if (c == '\0' || (c < 32 && c != '\t' && c != '\r' && c != '\n')) {
            return true;
        }
    }
    return false;
}

bool IsReservedWindowsName(std::string name) {
    // Strip extension
    size_t dotPos = name.find('.');
    if (dotPos != std::string::npos) {
        name = name.substr(0, dotPos);
    }
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    static const std::array<const char*, 22> reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    for (const auto* r : reserved) {
        if (name == r) return true;
    }
    return false;
}
} // namespace

PathResult PathSanitizer::Canonicalize(const std::filesystem::path& inputPath) {
    if (inputPath.empty()) {
        return PathResult::Fail(PathError::EmptyPath);
    }

    std::string u8Path = inputPath.string();
    if (ContainsNullOrControlChars(u8Path)) {
        return PathResult::Fail(PathError::InvalidCharacters);
    }

    // Check for reserved filenames in path components
    for (const auto& part : inputPath) {
        if (IsReservedWindowsName(part.string())) {
            return PathResult::Fail(PathError::ReservedDeviceName);
        }
    }

    try {
        std::error_code ec;
        // weakly_canonical resolves relative components and existing path prefixes without requiring the target file to exist
        std::filesystem::path canonical = std::filesystem::weakly_canonical(inputPath, ec);
        if (ec) {
            return PathResult::Fail(PathError::CanonicalizationFailed);
        }
        return PathResult::Ok(std::move(canonical));
    } catch (...) {
        return PathResult::Fail(PathError::CanonicalizationFailed);
    }
}

bool PathSanitizer::IsWithinBoundary(const std::filesystem::path& targetPath, const std::filesystem::path& rootBoundary) {
    std::error_code ec;
    std::filesystem::path canonicalTarget = std::filesystem::weakly_canonical(targetPath, ec);
    if (ec) return false;

    std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(rootBoundary, ec);
    if (ec) return false;

    auto rootStr = canonicalRoot.string();
    auto targetStr = canonicalTarget.string();

#if defined(_WIN32)
    // Windows paths are case-insensitive
    std::transform(rootStr.begin(), rootStr.end(), rootStr.begin(), ::tolower);
    std::transform(targetStr.begin(), targetStr.end(), targetStr.begin(), ::tolower);
#endif

    // Normalize slashes
    std::replace(rootStr.begin(), rootStr.end(), '\\', '/');
    std::replace(targetStr.begin(), targetStr.end(), '\\', '/');

    if (!rootStr.empty() && rootStr.back() != '/') {
        rootStr += '/';
    }

    return targetStr.starts_with(rootStr) || (targetStr + '/') == rootStr;
}

bool PathSanitizer::IsSafeFileName(const std::string& fileName) {
    if (fileName.empty() || fileName == "." || fileName == "..") {
        return false;
    }
    if (ContainsNullOrControlChars(fileName)) {
        return false;
    }
    if (IsReservedWindowsName(fileName)) {
        return false;
    }
    // Prohibit path separators and alternate data streams (':')
    for (char c : fileName) {
        if (c == '/' || c == '\\' || c == ':') {
            return false;
        }
    }
    return true;
}

std::string PathSanitizer::GetErrorMessage(PathError error) {
    switch (error) {
        case PathError::None: return "No error.";
        case PathError::EmptyPath: return "Path is empty.";
        case PathError::InvalidCharacters: return "Path contains null or illegal control characters.";
        case PathError::TraversalAttempt: return "Directory traversal attempted.";
        case PathError::OutsideBoundary: return "Path is outside the permitted boundary directory.";
        case PathError::ReservedDeviceName: return "Path uses a reserved OS device name.";
        case PathError::CanonicalizationFailed: return "Path canonicalization failed.";
        default: return "Unknown path error.";
    }
}

} // namespace pdfinity::security
