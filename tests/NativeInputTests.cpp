#include "Test.h"
#include "ToggleSprint/NativeInput.h"
#include "sdk/ModMenuBinding.h"
#include <array>
#include <cstring>
#include <limits>
#include <vector>
using namespace sprint;
namespace {
std::vector<int> calls;int failCreate{},failEnable{},creates{},enables{};
int init(){calls.push_back(1);return 0;}
int create(void* p,void*,void** o){++creates;calls.push_back(10+static_cast<int>(reinterpret_cast<std::uintptr_t>(p)));*o=p;return creates==failCreate?5:0;}
int enable(void* p){++enables;calls.push_back(20+static_cast<int>(reinterpret_cast<std::uintptr_t>(p)));return enables==failEnable?5:0;}
int disable(void* p){calls.push_back(30+static_cast<int>(reinterpret_cast<std::uintptr_t>(p)));return 0;}
int removeHook(void* p){calls.push_back(40+static_cast<int>(reinterpret_cast<std::uintptr_t>(p)));return 0;}
int uninit(){calls.push_back(2);return 0;}
std::array<void*,11> seen{};unsigned invoked{};bool sprintBit{};
std::uintptr_t __fastcall original(void* a,std::uint32_t* b,void* c,void* d,void* e,void* f,void* g,void* h,void* i,void* j,void* k){
    ++invoked;seen={a,b,c,d,e,f,g,h,i,j,k};auto m=static_cast<unsigned char*>(e);
    sprintBit=(m[96]&2)!=0;m[97]^=0x10;m[96]|=0x40;return 0x123456789ABCDEF0;
}
std::uintptr_t __fastcall singleOriginal(void* a,void* b,std::uint32_t* c){
    return original(a,c,nullptr,nullptr,b,nullptr,nullptr,nullptr,seen[8],seen[9],seen[10]);
}
std::array<void*,6> rawSeen{};unsigned rawInvoked{};
// Literal native conversion outputs, independent of RunRawInput's qualification.
// Keyboard Up has zero analog axes but a combined native vector (0, 1).
std::array<float,2> nativeMovement{0,1};
std::uintptr_t __fastcall rawOriginal(void* a,void* b,void* c,void* d,void* e,void* f){
    ++rawInvoked;rawSeen={a,b,c,d,e,f};
    memcpy(d,static_cast<unsigned char*>(b)+16,15);
    memcpy(static_cast<unsigned char*>(d)+16,nativeMovement.data(),8);
    return 0xFEDCBA9876543210;
}
}
int main(){
    // Breaks caught: enabling before all creation, missing rollback, wrong teardown ordering.
    HookApi api{init,create,enable,disable,removeHook,uninit};
    void* o1{},*o2{};HookSpec specs[]{{reinterpret_cast<void*>(1),reinterpret_cast<void*>(3),&o1},{reinterpret_cast<void*>(2),reinterpret_cast<void*>(4),&o2}};
    std::string error;HookSet set;
    Check(set.Install(api,specs,error),"install two exact targets");
    Check(calls==std::vector<int>{1,11,12,21,22},"all creation before enabling");
    set.Remove();Check(calls==std::vector<int>{1,11,12,21,22,32,31},"published hook teardown retains trampoline storage");
    auto n=calls.size();set.Remove();Check(calls.size()==n,"idempotent remove");
    // Runtime shutdown must not free a trampoline under an in-flight native call.
    HookSet shutdownSet;
    calls.clear();creates=enables=0;Check(shutdownSet.Install(api,specs,error),"shutdown fixture installed");
    shutdownSet.Disable();Check(calls==std::vector<int>{1,11,12,21,22,32,31},"disable retains created trampoline storage");
    shutdownSet.Disable();Check(calls.size()==7,"repeated disable inert");shutdownSet.Remove();
    for(int stage=1;stage<=2;++stage){
        HookSet failed;
        calls.clear();creates=enables=0;failCreate=stage;
        Check(!failed.Install(api,specs,error),"create failure refuses");
        Check(enables==0,"partial create never enabled");
        Check(!calls.empty() && calls.back()==2,"create rollback uninitializes");
        Check(calls== (stage==1?std::vector<int>{1,11,2}:std::vector<int>{1,11,12,41,2}),"created targets rolled back only");
    }
    failCreate=0;
    for(int stage=1;stage<=2;++stage){
        HookSet failed;
        calls.clear();creates=enables=0;failEnable=stage;
        Check(!failed.Install(api,specs,error),"enable failure refuses");
        Check(calls== (stage==1?std::vector<int>{1,11,12,21,42,41,2}:std::vector<int>{1,11,12,21,22,31}),"partial enable rolls back entries without freeing published trampoline");
    }
    // Breaks caught: late synthetic sprint state, clobbered jump/sneak input,
    // stale intent after physical refresh, and lost native arguments/results.
    std::array<unsigned char,100> move{};std::array<unsigned char,40> request{};
    std::array<unsigned char,80> gameplay{};std::array<unsigned char,368> actions{};
    std::uint32_t entity=7;Config cfg;SprintPolicy policy;
    SprintFrame frame{true,true,true,false,true,true,false,17,1};
    SprintDecision decision;
    auto run=[&](){return RunBulk(original,decision,reinterpret_cast<void*>(1),&entity,reinterpret_cast<void*>(3),reinterpret_cast<void*>(4),move.data(),reinterpret_cast<void*>(6),reinterpret_cast<void*>(7),reinterpret_cast<void*>(8),request.data(),gameplay.data(),actions.data());};
    cfg.mode=SprintMode::Force;run();
    Check(!sprintBit,"regression: late sprint consumer must not invent an input absent from raw/processed state");
    policy.Reset();cfg.mode=SprintMode::Toggle;move.fill(0);invoked=0;
    Check(run()==0x123456789ABCDEF0,"full width return preserved");Check(!sprintBit,"toggle initial frame vanilla");
    frame.toggleKeyDown=true;move.fill(0);decision={true,true,true,false};run();Check(!sprintBit,"latched consumer does not synthesize derived input");
    Check(move[96]==0x40 && move[97]==0x10,"original consumer effects preserved");
    Check(seen[0]==reinterpret_cast<void*>(1) && seen[1]==&entity && seen[2]==reinterpret_cast<void*>(3) && seen[3]==reinterpret_cast<void*>(4) && seen[4]==move.data() && seen[5]==reinterpret_cast<void*>(6) && seen[6]==reinterpret_cast<void*>(7) && seen[7]==reinterpret_cast<void*>(8) && seen[8]==request.data() && seen[9]==gameplay.data() && seen[10]==actions.data(),"all eleven ABI arguments preserved");
    // Breaks caught: unlatch-only leaves the existing vanilla sprint running;
    // the same physical Ctrl press can otherwise restart it on the next frame.
    frame.toggleKeyDown=true;move.fill(0);move[96]=2;decision={false,false,true,true};
    request.fill(0x5a);gameplay.fill(0x5a);actions.fill(0x5a);run();
    Check(sprintBit,"stop consumer receives unaltered coherent input");
    Check(request[10]==1 && gameplay[25]==0 && actions[1]==0x5e,"second press issues native stop request and event");
    Check(move[96]==0x42,"genuine input restored after call");
    for(unsigned index=0;index<request.size();++index)if(index!=10)Check(request[index]==0x5a,"stop preserves neighboring request bytes");
    for(unsigned index=0;index<gameplay.size();++index)if(index!=25)Check(gameplay[index]==0x5a,"stop preserves neighboring gameplay bytes");
    for(unsigned index=0;index<actions.size();++index)if(index!=1)Check(actions[index]==0x5a,"stop preserves neighboring action bytes");
    request[10]=0;actions[1]=0;decision={};run();
    Check(sprintBit && request[10]==0 && actions[1]==0,"no repeated stop event without a fresh queued unlatch");
    cfg.mode=SprintMode::Force;move.fill(0);run();Check(!sprintBit,"Force consumer never invents late sprint state");
    move[96]=2;run();Check(sprintBit && (move[96]&2),"genuine native sprint input not cleared");
    Check(invoked==6,"original once per consumer call");
    // Both native dispatch routes preserve input and use the same stop outputs.
    policy.Reset();cfg.mode=SprintMode::Toggle;frame={true,true,true,false,true,true,false,17,1};
    auto runSingle=[&](){return RunSingle(singleOriginal,decision,move.data(),request.data(),gameplay.data(),actions.data(),reinterpret_cast<void*>(1),move.data(),&entity);};
    Check(runSingle()==0x123456789ABCDEF0,"single original return preserved");
    frame.toggleKeyDown=true;move[96]=0;decision={true,true,true,false};runSingle();Check(!sprintBit && move[96]==0x40,"single consumer never synthesizes a sprint bit");
    move[96]=2;decision={false,false,true,true};
    request[10]=0;gameplay[25]=1;actions[1]=0x80;runSingle();
    Check(sprintBit && move[96]==0x42,"single stop preserves native input");
    Check(request[10]==1 && gameplay[25]==0 && actions[1]==0x84,"single stop uses same native request path");
    frame.blockingUi=true;move[96]=2;request[10]=0;actions[1]=0;decision={};runSingle();
    Check(sprintBit && request[10]==0 && actions[1]==0,"single invalid context passes original input unchanged");
    std::array<unsigned char,24> raw{};
    policy.Reset();cfg.mode=SprintMode::Toggle;frame={true,true,true,false,true,false,false,17,1};
    move.fill(0x5a);raw.fill(0xa5);
    // Physical JumpDown, SneakDown and Up; sprint is absent.
    std::uint32_t physical=0x2081;float x=0,y=0;
    memcpy(move.data()+16,&physical,4);memcpy(move.data()+20,&x,4);memcpy(move.data()+24,&y,4);
    auto physicalMove=move;
    auto rawRun=[&](){return RunRawInput(rawOriginal,policy,cfg,frame,decision,reinterpret_cast<void*>(1),move.data(),reinterpret_cast<void*>(3),raw.data(),reinterpret_cast<void*>(5),reinterpret_cast<void*>(6));};
    Check(rawRun()==0xFEDCBA9876543210,"raw full-width result preserved");
    Check(rawSeen==std::array<void*,6>{reinterpret_cast<void*>(1),move.data(),reinterpret_cast<void*>(3),raw.data(),reinterpret_cast<void*>(5),reinterpret_cast<void*>(6)},"all six raw ABI arguments preserved");
    Check(!decision.requestSprint && raw[1]==0x20,"fresh frame has no phantom latch");
    frame.toggleKeyDown=true;rawRun();
    Check(decision.toggled && decision.latched && decision.requestSprint && raw[1]==0x21,"fresh toggle changes raw SprintDown before consumers");
    constexpr std::array<unsigned char,24> expectedRaw{0x81,0x21,0,0,0,0,0,0,0,0,0,0,0x5a,0x5a,0x5a,0xa5,0,0,0,0,0,0,0x80,0x3f};
    for(unsigned i=0;i<raw.size();++i)Check(raw[i]==expectedRaw[i],"raw injection preserves independently derived jump/sneak/movement/auxiliary bytes");
    Check(move==physicalMove,"physical source including derived sprint/jump flags is never changed");
    frame.toggleKeyDown=false;rawRun();Check(raw[1]==0x21 && !decision.toggled,"latched intent reapplied after original physical refresh");
    frame.toggleKeyDown=true;rawRun();Check(raw[1]==0x20 && decision.toggled && !decision.latched,"second press clears raw input and queues a stop");
    physical|=0x100;memcpy(move.data()+16,&physical,4);rawRun();
    Check(raw[1]==0x20 && !decision.toggled,"held unlatch suppresses only raw sprint until release");
    frame.toggleKeyDown=false;rawRun();Check(raw[1]==0x21,"independent physical sprint resumes after release");
    physical&=~0x100u;memcpy(move.data()+16,&physical,4);cfg.mode=SprintMode::Force;rawRun();Check(raw[1]==0x21,"Force requests raw forward sprint");
    for(unsigned gate=0;gate<6;++gate){
        auto valid=frame;
        if(gate==0)frame.gameplayActive=false;
        if(gate==1)frame.localPlayer=false;
        if(gate==2)frame.focused=false;
        if(gate==3)frame.blockingUi=true;
        if(gate==4)frame.alive=false;
        if(gate==5)frame.playerToken=0;
        rawRun();Check(raw[1]==0x20 && !decision.requestSprint,"invalid gameplay context leaves freshly copied raw input alone");frame=valid;
    }
    for(float forward:{0.0f,-1.0f,.4f}){nativeMovement={0,forward};rawRun();Check(raw[1]==0x20,"idle backward and subthreshold remain vanilla");}
    nativeMovement={0,1};cfg.mode=SprintMode::Off;rawRun();Check(raw[1]==0x20,"Off removes no genuine input and adds none");
    physical|=0x100;memcpy(move.data()+16,&physical,4);rawRun();Check(raw[1]==0x21,"Off preserves genuine physical sprint");
    cfg.mode=SprintMode::Force;nativeMovement={0,0};rawRun();Check(raw[1]==0x21,"idle Force leaves independent vanilla input intact");
    Check(rawInvoked==19,"raw original called once for every admitted processing step");
    // Breaks caught: analog-only qualification misses keyboard directions;
    // stale analog values override a blocked native result; malformed outputs
    // or backward/sideways intent accidentally request sprint.
    struct MovementCase {std::uint32_t flags;float analogX,analogY,nativeX,nativeY;bool sprint;char const* label;};
    float const nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    MovementCase const movements[]{
        {0x2000,0,0,0,1,true,"keyboard-only Up requests sprint with zero analog axes"},
        {0xa000,0,0,.70710677f,.70710677f,true,"keyboard forward diagonal requests sprint"},
        {0x12000,0,0,-.70710677f,.70710677f,true,"opposite forward diagonal requests sprint"},
        {0x4000,0,0,0,-1,false,"keyboard backward stays vanilla"},
        {0x8000,0,0,1,0,false,"keyboard sideways stays vanilla"},
        {0x6000,0,0,0,0,false,"opposed Up and Down stay vanilla"},
        {0,0,0,0,0,false,"idle stays vanilla"},
        {0,0,1,0,1,true,"analog forward uses the native result"},
        {0,0,.4f,0,.4f,false,"subthreshold analog stays vanilla"},
        {0x2001,0,1,0,.3f,false,"native sneak scaling cannot be replaced by unscaled analog"},
        {0,0,1,0,0,false,"zero native result overrides nonzero raw analog"},
        {0,0,0,nan,1,false,"NaN native horizontal refused"},
        {0,0,0,0,nan,false,"NaN native forward refused"},
        {0,0,0,inf,1,false,"infinite native horizontal refused"},
        {0,0,0,0,inf,false,"infinite native forward refused"}
    };
    for(auto const& input:movements){
        policy.Reset();move.fill(0);raw.fill(0xa5);frame={true,true,true,false,true,false,false,17,cfg.revision};
        memcpy(move.data()+16,&input.flags,4);memcpy(move.data()+20,&input.analogX,4);memcpy(move.data()+24,&input.analogY,4);
        nativeMovement={input.nativeX,input.nativeY};auto before=move;auto priorCalls=rawInvoked;
        rawRun();Check(decision.requestSprint==input.sprint,input.label);
        std::uint32_t actualFlags{};memcpy(&actualFlags,raw.data(),4);
        Check(actualFlags==(input.flags|(input.sprint?0x100u:0u)),"only native raw sprint intent may change");
        Check(move==before && !memcmp(raw.data()+16,nativeMovement.data(),8),"physical and combined native movement remain untouched");
        Check(rawInvoked==priorCalls+1,"one original conversion call per movement case");
    }
    // Exact registry/sparse fixtures: reject stale entity generations and malformed hash chains.
    std::array<std::uintptr_t,60> registry{};std::array<std::uint64_t,4> buckets{0,UINT64_MAX,UINT64_MAX,UINT64_MAX};
    std::array<std::uintptr_t,4> node{};std::array<std::uintptr_t,11> pool{};
    std::array<std::uint32_t,2048> sparse{};sparse.fill(0xffffffff);sparse[7]=0;
    std::uintptr_t page=reinterpret_cast<std::uintptr_t>(sparse.data());
    pool[1]=reinterpret_cast<std::uintptr_t>(&page);pool[2]=pool[1]+8;
    std::uintptr_t payload=reinterpret_cast<std::uintptr_t>(move.data());pool[10]=reinterpret_cast<std::uintptr_t>(&payload);
    registry[15]=reinterpret_cast<std::uintptr_t>(buckets.data());registry[16]=registry[15]+32;
    registry[19]=reinterpret_cast<std::uintptr_t>(node.data());registry[20]=registry[19]+32;
    node={UINT64_MAX,0xACB47B38,reinterpret_cast<std::uintptr_t>(pool.data()),0};
    Check(FindPool(reinterpret_cast<std::uintptr_t>(registry.data()),0xACB47B38)==reinterpret_cast<std::uintptr_t>(pool.data()),"exact registry tag lookup");
    Check(PoolContains(reinterpret_cast<std::uintptr_t>(pool.data()),7),"current generation member");
    Check(!PoolContains(reinterpret_cast<std::uintptr_t>(pool.data()),7|0x40000),"stale generation rejected");
    Check(Component(reinterpret_cast<std::uintptr_t>(pool.data()),7,100)==payload,"component page stride");
    Check(!Component(reinterpret_cast<std::uintptr_t>(pool.data()),8,100),"absent sparse component");
    node[0]=0;Check(!FindPool(reinterpret_cast<std::uintptr_t>(registry.data()),0x1234),"hash cycle bounded");
    Check(!PoolContains(0,7) && !FindPool(0,0),"null context refused");
    // IDA 6698E10 passes a SneakingComponent sparse reference, not the query's
    // LocalPlayer filter. Catch rejecting every real callback or admitting a
    // foreign registry/remote entity just because payload addresses look valid.
    std::array<std::array<std::uintptr_t,4>,4> inputNodes{};
    std::array<std::uintptr_t,11> movePool=pool,rawPool=pool,sneakPool{};
    std::uintptr_t movePayload=reinterpret_cast<std::uintptr_t>(move.data()),rawPayload=reinterpret_cast<std::uintptr_t>(raw.data());
    movePool[10]=reinterpret_cast<std::uintptr_t>(&movePayload);rawPool[10]=reinterpret_cast<std::uintptr_t>(&rawPayload);
    std::array<std::uint32_t,4> hashes{0xacb47b38,0x018b1887,0x3613e513,0x8918332c};
    std::array<std::uintptr_t,4> inputPools{reinterpret_cast<std::uintptr_t>(pool.data()),reinterpret_cast<std::uintptr_t>(movePool.data()),reinterpret_cast<std::uintptr_t>(rawPool.data()),reinterpret_cast<std::uintptr_t>(sneakPool.data())};
    for(unsigned i=0;i<4;++i)inputNodes[i]={i==3?UINT64_MAX:i+1,hashes[i],inputPools[i],0};
    registry[15]=reinterpret_cast<std::uintptr_t>(buckets.data());registry[16]=registry[15]+8;buckets[0]=0;
    registry[19]=reinterpret_cast<std::uintptr_t>(inputNodes.data());registry[20]=registry[19]+128;
    std::array<std::uintptr_t,2> entityRef{inputPools[3],7};std::uint32_t observedEntity{};
    auto match=[&](){return MatchRawInput(reinterpret_cast<std::uintptr_t>(registry.data()),move.data(),raw.data(),entityRef.data(),observedEntity);};
    Check(match() && observedEntity==7,"real Sneaking sparse ref admits exact local raw/move payloads");
    entityRef[0]=inputPools[0];Check(!match(),"local filter cannot substitute for actual callback sparse ref");entityRef[0]=inputPools[3];
    entityRef[1]=7|0x40000;Check(!match(),"stale callback entity generation refused");entityRef[1]=7;
    std::array<std::uint32_t,2048> absent{};absent.fill(UINT32_MAX);
    std::uintptr_t absentPage=reinterpret_cast<std::uintptr_t>(absent.data());auto oldSparse=pool[1];
    pool[1]=reinterpret_cast<std::uintptr_t>(&absentPage);pool[2]=pool[1]+8;
    Check(!match(),"remote callback without local tag refused");pool[1]=oldSparse;pool[2]=oldSparse+8;
    ++rawPayload;Check(!match(),"raw payload from a different entity refused");--rawPayload;
    ++movePayload;Check(!match(),"move payload from a different entity refused");--movePayload;
    Check(!MatchRawInput(0,move.data(),raw.data(),entityRef.data(),observedEntity),"no enclosing synchronous registry refused");
    // A UI can stop raw callbacks while the sprint consumer keeps ticking.
    // Its observation must suspend UI output, not clear the retained latch.
    for(unsigned invalid=0;invalid<7;++invalid){
        SprintPolicy isolated;Config settings;SprintFrame live{true,true,true,false,true,true,false,1,1};
        live.revision=settings.revision;isolated.Step(SprintMode::Toggle,live);live.toggleKeyDown=true;isolated.Step(SprintMode::Toggle,live);
        auto blocked=live;bool pending=true;
        if(invalid==0)blocked.gameplayActive=false;
        if(invalid==1)blocked.localPlayer=false;
        if(invalid==2)blocked.focused=false;
        if(invalid==3)blocked.blockingUi=true;
        if(invalid==4)blocked.alive=false;
        if(invalid==5)blocked.playerToken=0;
        if(invalid==6)settings.mode=SprintMode::Off;
        auto out=ConsumeStop(isolated,pending,settings.revision,settings,blocked);
        Check(!pending && !out.toggled,"invalid consumer cannot issue a queued stop");
        live.toggleKeyDown=false;auto after=isolated.Step(SprintMode::Toggle,live);
        bool const temporary=invalid==0 || invalid==2 || invalid==3;
        Check(after.latched==temporary && after.requestSprint==temporary,"consumer retains latch for UI/focus only; death/invalid/off clear it without a raw callback");
    }
    // Consumer-only pause before the next raw callback: UI key presses must not
    // queue a stop or mutate raw/physical input; closing the UI resumes at once.
    for(unsigned gate=0;gate<3;++gate){
        policy.Reset();cfg.mode=SprintMode::Toggle;frame={true,true,true,false,true,true,false,17,cfg.revision};
        move.fill(0);physical=0x2000;memcpy(move.data()+16,&physical,4);nativeMovement={0,1};rawRun();
        frame.toggleKeyDown=true;rawRun();Check(decision.latched && raw[1]==0x21,"UI sequence starts with a real raw latch");
        bool staleStop=true;auto paused=frame;
        if(gate==0)paused.gameplayActive=false;
        if(gate==1)paused.blockingUi=true;
        if(gate==2)paused.focused=false;
        for(unsigned tick=0;tick<3;++tick){
            auto stop=ConsumeStop(policy,staleStop,cfg.revision,cfg,paused);
            Check(!staleStop && !stop.toggled,"consumer drops stale stop while UI blocks raw processing");
        }
        frame=paused;auto physicalBefore=move;rawRun();
        Check(decision.latched && !decision.requestSprint && !decision.inhibitSprintInput && raw[1]==0x20,"raw UI frame is pass-through while retaining latch");
        Check(move==physicalBefore,"UI pause never edits physical movement");
        frame={true,true,true,false,true,true,true,17,cfg.revision};rawRun();
        Check(decision.latched && decision.requestSprint && !decision.toggled && raw[1]==0x21,"raw resumes retained sprint even with Ctrl held across UI close");
        frame.toggleKeyDown=false;rawRun();frame.toggleKeyDown=true;rawRun();
        Check(!decision.latched && decision.toggled && raw[1]==0x20,"next fresh Ctrl after UI unsets latch once");
    }
    bool pending=true;cfg.mode=SprintMode::Toggle;frame={true,true,true,false,true,true,false,17,cfg.revision};
    Check(ConsumeStop(policy,pending,cfg.revision,cfg,frame).toggled && !pending,"fresh pending stop consumed once");
    Check(!ConsumeStop(policy,pending,cfg.revision,cfg,frame).toggled,"queued stop cannot replay");
    pending=true;Check(!ConsumeStop(policy,pending,cfg.revision+1,cfg,frame).toggled && !pending,"config change discards stale queued stop");
    // Exercise the physical-binding/policy/raw-output boundary with all eight
    // modifier masks. Expectations are literal sprint output, not tracker state.
    // The OS/native observation remains outside this offline fixture.
    for(auto token:{"K","CTRL+K","SHIFT+K","CTRL+SHIFT+K","LALT+K",
                    "CTRL+LALT+K","SHIFT+LALT+K","CTRL+SHIFT+LALT+K"}){
        auto parsed=ParseConfig(std::string("{\"toggleKey\":\"")+token+"\"}",error);
        Check(parsed.has_value(),"combination input fixture parses");if(!parsed)continue;
        cfg=*parsed;policy.Reset();modmenu::binding::Tracker tracker;
        modmenu::binding::Binding binding{cfg.toggleVk,cfg.toggleModifiers};
        modmenu::binding::Snapshot keys{};
        frame={true,true,true,false,true,true,false,17,cfg.revision};move.fill(0);
        physical=0x2000;memcpy(move.data()+16,&physical,4);nativeMovement={0,1};
        auto physicalStep=[&](){
            frame.toggleKeyDown=tracker.Update(binding,keys,frame.localPlayer&&frame.alive&&frame.gameplayActive&&frame.focused&&!frame.blockingUi).held;
            rawRun();
        };
        auto modifiers=[&](unsigned mask){keys[162]=(mask&1)!=0;keys[160]=(mask&2)!=0;keys[164]=(mask&4)!=0;};
        physicalStep();modifiers(binding.modifiers);physicalStep();keys[75]=true;physicalStep();
        Check(decision.toggled && decision.latched && raw[1]==0x21,"exact fresh chord requests sprint through native raw boundary");
        physicalStep();Check(!decision.toggled && decision.latched,"held chord cannot repeat toggle");
        frame.blockingUi=true;physicalStep();
        Check(decision.latched && !decision.toggled && raw[1]==0x20,"UI suspends chord output without clearing sprint latch");
        frame.blockingUi=false;physicalStep();physicalStep();
        Check(decision.latched && !decision.toggled && raw[1]==0x21,"preheld chord across UI cannot unlatch");
        keys[75]=false;physicalStep();keys[75]=true;physicalStep();
        Check(decision.toggled && !decision.latched && raw[1]==0x20,"fresh chord after UI unlatches and clears only SprintDown");
        keys[75]=false;physicalStep();modifiers(binding.modifiers^1u);physicalStep();keys[75]=true;physicalStep();
        Check(!decision.toggled && !decision.latched && raw[1]==0x20,"wrong modifier mask cannot request sprint");
        modifiers(binding.modifiers);physicalStep();
        Check(!decision.toggled && !decision.latched,"late modifier cannot turn a held primary into a toggle");
        keys[75]=false;physicalStep();keys[75]=true;physicalStep();
        Check(decision.toggled && decision.latched,"release then exact fresh primary re-arms toggle");
        ++cfg.revision;physicalStep();physicalStep();
        Check(!decision.latched && !decision.toggled && raw[1]==0x20,"config revision clears latch without preheld-key relatch");
        if(binding.modifiers&4u){
            keys[75]=false;physicalStep();keys[164]=false;keys[165]=true;keys[75]=true;physicalStep();
            Check(!decision.latched && !decision.toggled,"Right Alt cannot substitute for Left Alt chord modifier");
        }
    }
    return Finish("NativeInput");
}
