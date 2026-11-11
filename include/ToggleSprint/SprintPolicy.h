#pragma once
#include "Config.h"
namespace sprint {
struct SprintFrame {
    // gameplayActive is the scene/input gate, not proof that the world ended.
    // A verified local player may still exist while an inventory/menu owns input.
    bool gameplayActive{},localPlayer{},focused{},blockingUi{},alive{},qualifyingMovement{},toggleKeyDown{};
    std::uint64_t playerToken{},revision{};
};
struct SprintDecision { bool requestSprint{},latched{},toggled{},inhibitSprintInput{}; };
class SprintPolicy {
    bool latched_{},previousKeyDown_{},keyPrimed_{};
    SprintMode previousMode_{SprintMode::Off};
    std::uint64_t playerToken_{},revision_{};
public:
    SprintDecision Step(SprintMode, SprintFrame const&) noexcept;
    void SuspendInput() noexcept;
    void Reset() noexcept;
};
}
