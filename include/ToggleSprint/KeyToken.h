#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace sprint {
std::optional<std::uint16_t> ParseKeyToken(std::string_view) noexcept;
std::vector<std::string> SupportedKeys();
}
