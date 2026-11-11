#pragma once
#include "Config.h"
#include <filesystem>
namespace sprint {
enum class ReloadResult {Unchanged,Applied,Rejected};
class ConfigFile {
    std::filesystem::path path_;std::string previous_;bool observed_{};
public:
    explicit ConfigFile(std::filesystem::path path):path_(std::move(path)){}
    ReloadResult Poll(ConfigState&,std::string&);
};
}
