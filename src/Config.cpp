#include "ToggleSprint/Config.h"
#include "../include/sdk/ModMenuBinding.h"
#include "ToggleSprint/KeyToken.h"
#include <json.hpp>
#include <set>
namespace sprint {
std::optional<Config> ParseConfig(std::string_view text,std::string& error) {
    try {
        if(text.size()>65536)throw std::runtime_error("Configuration exceeds 64 KiB");
        std::set<std::string> members;
        auto callback=[&](int depth,nlohmann::json::parse_event_t event,nlohmann::json& parsed) {
            if(depth>4)throw std::runtime_error("Configuration nesting invalid");
            if(event==nlohmann::json::parse_event_t::key && !members.insert(parsed.get<std::string>()).second)
                throw std::runtime_error("Duplicate config member");
            return true;
        };
        auto document=nlohmann::json::parse(text,callback);
        if(!document.is_object())throw std::runtime_error("Configuration must be an object");
        for(auto const& [key,value]:document.items()) {
            (void)value;
            if(key!="mode" && key!="toggleKey")throw std::runtime_error("Unknown config member");
        }
        Config next;
        if(document.contains("mode")) {
            if(!document["mode"].is_number_integer() || document["mode"]<0 || document["mode"]>2)
                throw std::runtime_error("mode must be an integer from 0 to 2");
            next.mode=static_cast<SprintMode>(document["mode"].get<int>());
        }
        if(document.contains("toggleKey")) {
            if(!document["toggleKey"].is_string())throw std::runtime_error("toggleKey must be a key token");
            next.toggleToken=document["toggleKey"].get<std::string>();
            auto vk=ParseKeyToken(next.toggleToken);
            if(!vk)throw std::runtime_error("Unsupported key token");
            next.toggleVk=*vk;
            if(auto b=modmenu::binding::ParseInput(next.toggleToken))next.toggleModifiers=b->modifiers;
        }
        error.clear();return next;
    } catch(std::exception const& e) {error=e.what();return {};}
}
bool ConfigState::Apply(std::string_view json,std::string& error) {
    auto next=ParseConfig(json,error);if(!next)return false;
    auto prior=current_.load();
    if(prior->mode==next->mode && prior->toggleVk==next->toggleVk && prior->toggleModifiers==next->toggleModifiers)return true;
    next->revision=prior->revision+1;
    current_.store(std::make_shared<Config const>(std::move(*next)));return true;
}
Config ConfigState::Snapshot() const { return *current_.load(); }
}
