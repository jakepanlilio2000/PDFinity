// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <filesystem>
#include <string>

namespace pdfinity::security {

enum class PathError {
    None,
    EmptyPath,
    InvalidCharacters,
    TraversalAttempt,
    OutsideBoundary,
    ReservedDeviceName,
    CanonicalizationFailed
};

struct PathResult {
    bool success{false};
    std::filesystem::path path;
    PathError err{PathError::None};

    [[nodiscard]] bool has_value() const noexcept { return success; }
    [[nodiscard]] explicit operator bool() const noexcept { return success; }
    [[nodiscard]] const std::filesystem::path& value() const noexcept { return path; }
    [[nodiscard]] PathError error() const noexcept { return err; }

    static PathResult Ok(std::filesystem::path p) {
        PathResult r;
        r.success = true;
        r.path = std::move(p);
        r.err = PathError::None;
        return r;
    }

    static PathResult Fail(PathError e) {
        PathResult r;
        r.success = false;
        r.err = e;
        return r;
    }
};

class PathSanitizer {
public:
    static PathResult Canonicalize(const std::filesystem::path& inputPath);
    
    static bool IsWithinBoundary(const std::filesystem::path& targetPath, const std::filesystem::path& rootBoundary);

    static bool IsSafeFileName(const std::string& fileName);

    static std::string GetErrorMessage(PathError error);
};

} // namespace pdfinity::security
