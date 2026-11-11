#include "ToggleSprint/SprintPolicy.h"
namespace sprint {
SprintDecision SprintPolicy::Step(SprintMode mode,SprintFrame const& f) noexcept {
    if(mode==SprintMode::Off || !f.localPlayer || !f.alive || !f.playerToken) {
        Reset();return {};
    }
    if(previousMode_!=mode || playerToken_!=f.playerToken || revision_!=f.revision) {
        Reset();previousMode_=mode;playerToken_=f.playerToken;revision_=f.revision;
    }
    if(!f.gameplayActive || !f.focused || f.blockingUi) {
        SuspendInput();return {false,latched_,false,false};
    }
    bool toggled=false;
    if(mode==SprintMode::Toggle && keyPrimed_ && f.toggleKeyDown && !previousKeyDown_) {
        latched_=!latched_;toggled=true;
    }
    previousKeyDown_=f.toggleKeyDown;keyPrimed_=true;
    return {f.qualifyingMovement && (mode==SprintMode::Force || latched_),latched_,toggled,
        mode==SprintMode::Toggle && !latched_ && f.toggleKeyDown};
}
void SprintPolicy::SuspendInput() noexcept { previousKeyDown_=false;keyPrimed_=false; }
void SprintPolicy::Reset() noexcept { *this=SprintPolicy{}; }
}
