#pragma once

#include <string_view>

namespace uncannyplumber {

// The port's semantic version string, from the CMake project version. Never empty.
[[nodiscard]] std::string_view version() noexcept;

}  // namespace uncannyplumber
