#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace sprint {
enum class GuardValidation { Runtime, DiskEvidence };
struct ByteGuard {
    std::uint32_t rva{}, end{};
    std::array<unsigned char,32> bytes{};
    GuardValidation validation{GuardValidation::Runtime};
};
struct Profile {
    char const* version{};
    std::array<char const*,2> hashes{};
    std::uint32_t timestamp{}, imageSize{}, launcherSize{}, hookRva{}, hookEnd{};
    std::vector<ByteGuard> guards;
};
Profile const& ExactProfile();
std::string Sha256(std::span<unsigned char const> bytes);
bool ValidateImage(std::span<unsigned char const> bytes, Profile const&, std::string& error);
bool ValidateLoadedImage(std::span<unsigned char const> mapped, Profile const&, std::string& error);
bool ValidateHost(Profile const&, std::uintptr_t& base, std::string& error);
}
