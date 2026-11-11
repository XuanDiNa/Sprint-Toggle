#include "Test.h"
#include "ToggleSprint/SprintPolicy.h"
using namespace sprint;
SprintFrame World() { return {true,true,true,false,true,true,false,42,1}; }
int main() {
    SprintPolicy p; auto f=World();
    Check(!p.Step(SprintMode::Toggle,f).latched,"initial release baseline");
    f.toggleKeyDown=true; auto d=p.Step(SprintMode::Toggle,f);
    Check(d.latched && d.toggled && d.requestSprint,"fresh press latches");
    d=p.Step(SprintMode::Toggle,f);
    Check(d.latched && !d.toggled,"held key cannot toggle twice");
    f.toggleKeyDown=false; Check(p.Step(SprintMode::Toggle,f).requestSprint,"release retains latch");
    f.qualifyingMovement=false; d=p.Step(SprintMode::Toggle,f);
    Check(d.latched && !d.requestSprint,"stop movement retains latch without idle sprint");
    f.qualifyingMovement=true; Check(p.Step(SprintMode::Toggle,f).requestSprint,"resume movement");
    f.toggleKeyDown=true;d=p.Step(SprintMode::Toggle,f);
    Check(!d.latched && d.toggled && d.inhibitSprintInput,"second press unlatches and inhibits held native key");
    d=p.Step(SprintMode::Toggle,f);Check(d.inhibitSprintInput && !d.toggled,"held off key inhibits without repeating event");
    f.toggleKeyDown=false;Check(!p.Step(SprintMode::Toggle,f).inhibitSprintInput,"released key leaves native input alone");
    f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
    f.blockingUi=true;d=p.Step(SprintMode::Toggle,f);
    Check(d.latched && !d.requestSprint && !d.toggled && !d.inhibitSprintInput,"UI suspends output without clearing latch");
    f.blockingUi=false;d=p.Step(SprintMode::Toggle,f);
    Check(d.latched && d.requestSprint && !d.toggled,"held key returning from UI resumes without toggling");
    f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;
    d=p.Step(SprintMode::Toggle,f);Check(!d.latched && d.toggled,"fresh press after UI releases retained latch");
    f.focused=false;Check(!p.Step(SprintMode::Force,f).requestSprint,"unfocused force blocked");
    Check(!p.Step(SprintMode::Toggle,f).inhibitSprintInput,"invalid context cannot inhibit native input");
    f=World();Check(p.Step(SprintMode::Force,f).requestSprint,"force requests movement without key");
    f.qualifyingMovement=false;Check(!p.Step(SprintMode::Force,f).requestSprint,"force idle is vanilla");
    f=World();Check(!p.Step(SprintMode::Off,f).requestSprint,"off passes through");
    f.toggleKeyDown=true;Check(!p.Step(SprintMode::Toggle,f).latched,"mode change held key primes only");
    f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
    ++f.revision;Check(!p.Step(SprintMode::Toggle,f).latched,"hot bind revision clears latch");
    f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
    ++f.playerToken;Check(!p.Step(SprintMode::Toggle,f).latched,"new actor clears latch");
    for(int gate=0;gate<3;++gate) {
        p.Reset();f=World();p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
        if(gate==0)f.localPlayer=false;if(gate==1)f.alive=false;if(gate==2)f.playerToken=0;
        Check(!p.Step(SprintMode::Toggle,f).latched,"invalid world/life clears latch");
    }
    // Breaks caught: UI/focus clearing the latch, accepting Ctrl inside a UI,
    // a held key on return becoming a phantom toggle, and a paused first frame
    // preventing the first real gameplay press from latching.
    for(unsigned gate=0;gate<3;++gate){
        for(bool heldOnReturn:{false,true}){
            p.Reset();f=World();p.Step(SprintMode::Toggle,f);
            f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
            auto paused=f;
            if(gate==0)paused.gameplayActive=false;
            if(gate==1)paused.blockingUi=true;
            if(gate==2)paused.focused=false;
            bool pausedPassThrough=true;
            for(unsigned tick=0;tick<200;++tick){
                paused.toggleKeyDown=(tick%2)==0;
                d=p.Step(SprintMode::Toggle,paused);
                pausedPassThrough=pausedPassThrough && d.latched && !d.requestSprint && !d.toggled && !d.inhibitSprintInput;
            }
            Check(pausedPassThrough,"UI/focus never toggles or writes input and retains existing latch over 200 frames");
            f.toggleKeyDown=heldOnReturn;d=p.Step(SprintMode::Toggle,f);
            Check(d.latched && d.requestSprint && !d.toggled,"first gameplay frame resumes old latch without phantom press");
            f.toggleKeyDown=false;d=p.Step(SprintMode::Toggle,f);
            Check(d.latched && !d.toggled,"release after UI retains latch");
            f.toggleKeyDown=true;d=p.Step(SprintMode::Toggle,f);
            Check(!d.latched && d.toggled && !d.requestSprint,"next fresh gameplay press alone releases latch");
        }
        p.Reset();f=World();f.blockingUi=true;f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
        f.blockingUi=false;d=p.Step(SprintMode::Toggle,f);
        Check(!d.latched && !d.toggled,"starting while a UI owns Ctrl cannot latch on return");
        f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;
        Check(p.Step(SprintMode::Toggle,f).latched,"fresh gameplay press after initially paused frame works");
    }
    // A real reset/config/player change still wins over paused retention.
    for(unsigned change=0;change<5;++change){
        p.Reset();f=World();p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);
        f.blockingUi=true;p.Step(SprintMode::Toggle,f);
        if(change==0)++f.revision;
        if(change==1)++f.playerToken;
        if(change==2)f.alive=false;
        if(change==3)f.localPlayer=false;
        d=p.Step(change==4?SprintMode::Off:SprintMode::Toggle,f);
        Check(!d.latched && !d.requestSprint,"hard lifecycle or settings reset clears even while paused");
        f.blockingUi=false;f.alive=f.localPlayer=true;d=p.Step(SprintMode::Toggle,f);
        Check(!d.latched && !d.toggled,"hard reset never restores earlier paused latch");
    }
    // The native >500 ms same-identity gap uses this method because a paused
    // single-player game may provide no intervening UI frames at all.
    for(bool wasLatched:{false,true}){
        p.Reset();f=World();p.Step(SprintMode::Toggle,f);
        if(wasLatched){f.toggleKeyDown=true;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);}
        p.SuspendInput();p.SuspendInput();f.toggleKeyDown=true;d=p.Step(SprintMode::Toggle,f);
        Check(d.latched==wasLatched && !d.toggled && d.requestSprint==wasLatched,"same-player input gap retains latch and ignores unknown held-key edge");
        f.toggleKeyDown=false;p.Step(SprintMode::Toggle,f);f.toggleKeyDown=true;d=p.Step(SprintMode::Toggle,f);
        Check(d.latched!=wasLatched && d.toggled,"fresh press after gap still toggles exactly once");
    }
    p.Reset();f=World();p.Step(SprintMode::Toggle,f);
    for(int i=0;i<100;++i) Check(!p.Step(SprintMode::Toggle,f).latched,"unbound never toggles");
    return Finish("SprintPolicy");
}
