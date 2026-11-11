#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
namespace sprint {
enum class SprintMode : std::uint8_t { Off, Toggle, Force };
struct Config {
    SprintMode mode{SprintMode::Toggle};
    std::uint16_t toggleVk{0xA2};
    std::string toggleToken{"LCTRL"};
    std::uint64_t revision{1};
    unsigned toggleModifiers{};
};
std::optional<Config> ParseConfig(std::string_view, std::string&);
class ConfigState {
    std::atomic<std::shared_ptr<Config const>> current_{std::make_shared<Config>()};
public:
    bool Apply(std::string_view, std::string&);
    Config Snapshot() const;
};
}
