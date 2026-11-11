#include "ToggleSprint/NativeInput.h"
#include "../include/sdk/ModMenuBinding.h"
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstring>
namespace sprint {
bool HookSet::Install(HookApi const& api,std::span<HookSpec const> specs,std::string& error){
    if(initialized_ || specs.empty() || specs.size()>8 || !api.initialize || !api.create || !api.enable || !api.disable || !api.remove || !api.uninitialize){error="Invalid hook transaction";return false;}
    // Reserve before touching native hooks: no allocation is allowed mid-transaction.
    created_.reserve(specs.size());enabled_.reserve(specs.size());api_=api;
    int status=api_.initialize();
    if(status!=0 && status!=1){error="Hook initialization refused";return false;}
    initialized_=true;ownsInitialization_=status==0;
    for(auto const& s:specs){
        if(!s.target || !s.replacement || !s.original || api_.create(s.target,s.replacement,s.original)!=0){error="Hook creation refused";Remove();return false;}
        created_.push_back(s.target);
    }
    for(auto const& s:specs){
        if(api_.enable(s.target)!=0){error="Hook enabling refused";Remove();return false;}
        everEnabled_=true;
        enabled_.push_back(s.target);
    }
    error.clear();return true;
}
void HookSet::Disable() noexcept{
    if(!initialized_)return;
    for(auto it=enabled_.rbegin();it!=enabled_.rend();++it)api_.disable(*it);
    enabled_.clear();
}
void HookSet::Remove() noexcept{
    if(!initialized_)return;
    Disable();
    // Even a failed later enable may leave an earlier callback in flight.
    // Keep all published trampolines for this pinned module's process lifetime.
    if(everEnabled_)return;
    for(auto it=created_.rbegin();it!=created_.rend();++it)api_.remove(*it);
    if(ownsInitialization_)api_.uninitialize();
    enabled_.clear();created_.clear();initialized_=ownsInitialization_=false;
}
namespace {
void StopRequest(SprintDecision const& decision,void* request,void* gameplay,void* actions){
    if(decision.toggled && !decision.latched && request && gameplay && actions){
        // B1A4F00 normal stop branch: request+10=1, gameplay+25=0,
        // actions bit 0x400. B1AA300 consumes it through B17CAA0(false).
        static_cast<unsigned char*>(request)[10]=1;
        static_cast<unsigned char*>(gameplay)[25]=0;
        static_cast<unsigned char*>(actions)[1]|=4;
    }
}
}
std::uintptr_t RunRawInput(RawInputFn original,SprintPolicy& policy,Config const& cfg,SprintFrame frame,SprintDecision& decision,
    void* a,void* move,void* c,void* raw,void* e,void* f){
    auto result=original(a,move,c,raw,e,f);
    // The original has refreshed raw state fields and its native movement vector.
    // Amend only SprintDown. The remaining native stages derive movement,
    // jump and sprint state from this same input; no post-consumer restoration.
    if(raw){
        // 4495E0 merges digital directions when analog axes are zero, then
        // applies native normalization/scaling. 6688630 stores that result at
        // +16/+20. Reading +4/+8 alone loses ordinary keyboard-only movement.
        float x{},y{};memcpy(&x,static_cast<unsigned char*>(raw)+16,4);memcpy(&y,static_cast<unsigned char*>(raw)+20,4);
        frame.qualifyingMovement=std::isfinite(x) && std::isfinite(y) && y>0 &&
            std::hypot(x,y)>=0.70710677f && std::fabs(x)<=0.70710677f;
    }else frame.qualifyingMovement=false;
    frame.revision=cfg.revision;decision=policy.Step(cfg.mode,frame);
    if(raw && (decision.requestSprint || decision.inhibitSprintInput)){
        auto byte=static_cast<unsigned char*>(raw)+1;
        *byte=static_cast<unsigned char>((*byte&~1u)|(decision.requestSprint?1u:0u));
    }
    return result;
}
std::uintptr_t RunBulk(BulkFn original,SprintDecision const& decision,
    void* a,std::uint32_t* b,void* c,void* d,void* e,void* f,void* g,void* h,void* i,void* j,void* k){
    auto result=original(a,b,c,d,e,f,g,h,i,j,k);
    StopRequest(decision,i,j,k);return result;
}
std::uintptr_t RunSingle(SingleFn original,SprintDecision const& decision,
    void*,void* request,void* gameplay,void* actions,void* a,void* b,std::uint32_t* c){
    auto result=original(a,b,c);StopRequest(decision,request,gameplay,actions);return result;
}
SprintDecision ConsumeStop(SprintPolicy& policy,bool& pending,std::uint64_t decisionRevision,Config const& cfg,SprintFrame const& frame){
    if(cfg.mode==SprintMode::Off || decisionRevision!=cfg.revision || !frame.localPlayer ||
        !frame.alive || !frame.playerToken){
        policy.Reset();pending=false;return {};
    }
    if(!frame.gameplayActive || !frame.focused || frame.blockingUi){
        policy.SuspendInput();pending=false;return {};
    }
    bool stop=pending;pending=false;return {false,false,stop,false};
}
std::uintptr_t FindPool(std::uintptr_t root,std::uint32_t hash) noexcept {
    if(!root)return 0;
    __try {
        // IDA 476E930: registry map at root+48; 32-byte nodes with +8 hash/+16 pool.
        auto map=root+48;
        auto buckets=*reinterpret_cast<std::uintptr_t const*>(map+72),end=*reinterpret_cast<std::uintptr_t const*>(map+80);
        auto nodes=*reinterpret_cast<std::uintptr_t const*>(map+104),nodeEnd=*reinterpret_cast<std::uintptr_t const*>(map+112);
        if(!buckets || !nodes || end<=buckets || (end-buckets)%8 || nodeEnd<nodes || (nodeEnd-nodes)%32)return 0;
        auto count=(end-buckets)/8,nodeCount=(nodeEnd-nodes)/32;
        if(count>(1u<<20) || (count&(count-1)) || nodeCount>(1u<<20))return 0;
        auto index=reinterpret_cast<std::uint64_t const*>(buckets)[hash&(count-1)];
        for(unsigned chain=0;index!=UINT64_MAX && chain<128;++chain){
            if(index>=nodeCount)return 0;auto node=nodes+32*index;
            if(*reinterpret_cast<std::uint32_t const*>(node+8)==hash)return *reinterpret_cast<std::uintptr_t const*>(node+16);
            index=*reinterpret_cast<std::uint64_t const*>(node);
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){}
    return 0;
}
bool PoolContains(std::uintptr_t pool,std::uint32_t entity) noexcept {
    if(!pool)return false;
    __try {
        auto begin=*reinterpret_cast<std::uintptr_t const*>(pool+8),end=*reinterpret_cast<std::uintptr_t const*>(pool+16);
        auto index=(entity&0x3ffffu)>>11;
        if(!begin || end<begin || end-begin>128*8 || (end-begin)%8 || index>=(end-begin)/8)return false;
        auto page=reinterpret_cast<std::uintptr_t const*>(begin)[index];if(!page)return false;
        auto packed=reinterpret_cast<std::uint32_t const*>(page)[entity&0x7ff];
        return (packed^(entity&0xfffc0000u))<=0x3fffe;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
std::uintptr_t Component(std::uintptr_t pool,std::uint32_t entity,unsigned stride) noexcept {
    if(!stride || stride>4096 || !PoolContains(pool,entity))return 0;
    __try {
        auto sparse=*reinterpret_cast<std::uintptr_t const*>(pool+8);
        auto page=reinterpret_cast<std::uintptr_t const*>(sparse)[(entity&0x3ffffu)>>11];
        auto packed=reinterpret_cast<std::uint32_t const*>(page)[entity&0x7ff];
        auto pages=*reinterpret_cast<std::uintptr_t const*>(pool+80);if(!pages)return 0;
        auto payload=reinterpret_cast<std::uintptr_t const*>(pages)[(packed&0x3ffffu)>>7];
        return payload?payload+stride*(packed&0x7fu):0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool MatchRawInput(std::uintptr_t root,void* move,void* raw,void* entityRef,std::uint32_t& entity) noexcept {
    __try {
        if(!root || !entityRef)return false;
        auto ref=static_cast<std::uintptr_t const*>(entityRef);
        entity=static_cast<std::uint32_t>(ref[1]);
        // 6698E10 constructs this argument from SneakingComponent, while
        // LocalPlayer is a query filter; its sparse membership is checked separately.
        return ref[0]==FindPool(root,0x8918332c) && PoolContains(FindPool(root,0xacb47b38),entity) &&
            Component(FindPool(root,0x018b1887),entity,100)==reinterpret_cast<std::uintptr_t>(move) &&
            Component(FindPool(root,0x3613e513),entity,24)==reinterpret_cast<std::uintptr_t>(raw);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
namespace {
std::atomic<NativeStatus> status{NativeStatus::Inactive};std::atomic<bool> active{};
std::uintptr_t base{};ConfigState* config{};HookSet hooks;BulkFn bulk{};SingleFn single{};
InputDispatchFn inputDispatch{};RawInputFn rawInput{};
thread_local std::uintptr_t inputRoot{};
std::atomic<std::uint64_t> dispatchCount{},rawCount{},admittedCount{},requestCount{},jumpCount{},stopCount{};
bool pendingStop{};std::uint64_t decisionRevision{};
SRWLOCK stateLock=SRWLOCK_INIT;SprintPolicy policy;
std::uintptr_t lastRegistry{},lastActor{},lastClient{};std::uint32_t lastEntity{};
std::uint64_t generation{},lastFrame{};
struct Lock {
    bool held{TryAcquireSRWLockExclusive(&stateLock)!=0};
    ~Lock(){if(held)ReleaseSRWLockExclusive(&stateLock);}
};
bool IsLocal(std::uintptr_t root,std::uint32_t entity){return PoolContains(FindPool(root,0xacb47b38),entity);}
struct Observation {SprintFrame frame;bool observed{};};
Observation Observe(std::uintptr_t root,std::uint32_t entity,void* request) noexcept {
    Observation out;
    __try {
        auto owner=Component(FindPool(root,0x85b93800),entity,8);
        auto actor=owner?*reinterpret_cast<std::uintptr_t const*>(owner):0;
        if(!actor || *reinterpret_cast<std::uintptr_t const*>(actor)!=base+0xe8e1bc0)return out;
        auto client=*reinterpret_cast<std::uintptr_t const*>(actor+3440);
        if(!client || *reinterpret_cast<std::uintptr_t const*>(client)!=base+0xe9731b0)return out;
        using Getter=void*(__fastcall*)(void*);using Predicate=bool(__fastcall*)(void*);
        if(reinterpret_cast<Getter>(base+0x5daed30)(reinterpret_cast<void*>(client))!=reinterpret_cast<void*>(actor))return out;
        auto now=GetTickCount64();
        if(root!=lastRegistry || entity!=lastEntity || actor!=lastActor || client!=lastClient){
            policy.Reset();pendingStop=false;++generation;lastRegistry=root;lastEntity=entity;lastActor=actor;lastClient=client;
        }else if(now-lastFrame>500){
            // Menus (notably single-player pause) can stop all these callbacks.
            // Same freshly verified identity: retain latch, discard key edges
            // and queued output. A different identity still fully resets above.
            policy.SuspendInput();pendingStop=false;
        }
        lastFrame=now;
        auto& f=out.frame;f.localPlayer=true;f.playerToken=generation;
        f.alive=!PoolContains(FindPool(root,0x710a62c4),entity);
        f.gameplayActive=reinterpret_cast<Predicate>(base+0x5dc2140)(reinterpret_cast<void*>(client));
        f.blockingUi=!f.gameplayActive || *reinterpret_cast<unsigned char const*>(client+712)!=1;
        DWORD foregroundPid{};GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
        f.focused=foregroundPid==GetCurrentProcessId() && reinterpret_cast<Predicate>(base+0x5dc0c80)(reinterpret_cast<void*>(client));
        if(request){
            float x{},y{};memcpy(&x,request,4);memcpy(&y,static_cast<unsigned char*>(request)+4,4);
            f.qualifyingMovement=std::isfinite(x) && std::isfinite(y) && y>0 && std::hypot(x,y)>=0.70710677f &&
                std::fabs(x)<=0.70710677f && static_cast<unsigned char*>(request)[15]==1;
        }
        out.observed=true;
    }__except(EXCEPTION_EXECUTE_HANDLER){}
    return out;
}
std::uintptr_t Root(void* context,bool dispatch) noexcept {
    __try {
        if(!context)return 0;
        if(dispatch)context=reinterpret_cast<void**>(context)[1];
        return context?*reinterpret_cast<std::uintptr_t const*>(context):0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool ReadEntity(std::uint32_t* entity,std::uint32_t& value) noexcept {
    __try {if(!entity)return false;value=*entity;return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
std::uintptr_t __fastcall InputDispatchHook(void* a,void* b){
    ++dispatchCount;
    // 6693B20 synchronously invokes component callbacks. Nested dispatches
    // restore their caller's registry; other threads never inherit this scope.
    auto previous=inputRoot;inputRoot=active.load()?Root(b,false):0;
    auto result=inputDispatch(a,b);inputRoot=previous;return result;
}
std::uintptr_t __fastcall RawInputHook(void* a,void* b,void* c,void* d,void* e,void* f){
    ++rawCount;std::uint32_t entity{};
    if(!active.load() || !MatchRawInput(inputRoot,b,d,e,entity))return rawInput(a,b,c,d,e,f);
    Lock lock;if(!lock.held)return rawInput(a,b,c,d,e,f);
    static modmenu::binding::Tracker bindingTracker;
    auto observed=Observe(inputRoot,entity,nullptr);
    if(!observed.observed){bindingTracker.Reset();policy.Reset();pendingStop=false;return rawInput(a,b,c,d,e,f);}
    auto cfg=config->Snapshot();
    modmenu::binding::Binding binding{cfg.toggleVk,cfg.toggleModifiers};
    auto sample=modmenu::binding::Sample(binding,[](unsigned k){return (GetAsyncKeyState(static_cast<int>(k))&0x8000)!=0;});
    auto const& frame=observed.frame;
    observed.frame.toggleKeyDown=bindingTracker.Update(binding,sample,frame.localPlayer&&frame.alive&&frame.gameplayActive&&frame.focused&&!frame.blockingUi).held;
    SprintDecision decision;
    auto result=RunRawInput(rawInput,policy,cfg,observed.frame,decision,a,b,c,d,e,f);
    ++admittedCount;if(decision.requestSprint)++requestCount;
    if((static_cast<unsigned char*>(d)[0]&0x80)!=0)++jumpCount;
    if(decisionRevision!=cfg.revision || !observed.frame.gameplayActive || !observed.frame.focused ||
        observed.frame.blockingUi || !observed.frame.alive)pendingStop=false;
    decisionRevision=cfg.revision;
    if(decision.toggled)pendingStop=!decision.latched;
    return result;
}
SprintDecision TakeStop(Config const& cfg,SprintFrame const& frame){
    auto decision=ConsumeStop(policy,pendingStop,decisionRevision,cfg,frame);
    if(decision.toggled)++stopCount;return decision;
}
std::uintptr_t __fastcall BulkHook(void* a,std::uint32_t* b,void* c,void* d,void* e,void* f,void* g,void* h,void* i,void* j,void* k){
    // B1A5490, eleven integer/pointer Win64 arguments. No altered register ABI.
    auto root=Root(a,true);std::uint32_t entity{};
    if(!active.load() || !ReadEntity(b,entity) || !IsLocal(root,entity))return bulk(a,b,c,d,e,f,g,h,i,j,k);
    Lock lock;if(!lock.held)return bulk(a,b,c,d,e,f,g,h,i,j,k);
    auto observed=Observe(root,entity,i);
    if(!observed.observed){policy.Reset();pendingStop=false;return bulk(a,b,c,d,e,f,g,h,i,j,k);}
    auto decision=TakeStop(config->Snapshot(),observed.frame);
    return RunBulk(bulk,decision,a,b,c,d,e,f,g,h,i,j,k);
}
std::uintptr_t __fastcall SingleHook(void* a,void* b,std::uint32_t* c){
    // B1A8140 forwards *b to B1A8620 and directly invokes the same intent leaf.
    auto root=Root(b,false);std::uint32_t entity{};
    if(!active.load() || !ReadEntity(c,entity) || !IsLocal(root,entity))return single(a,b,c);
    Lock lock;if(!lock.held)return single(a,b,c);
    auto move=Component(FindPool(root,0x018b1887),entity,100);
    auto request=Component(FindPool(root,0x6b04a058),entity,40);
    auto gameplay=Component(FindPool(root,0xa406686e),entity,80);
    auto actions=Component(FindPool(root,0x3000f399),entity,368);
    auto observed=Observe(root,entity,reinterpret_cast<void*>(request));
    if(!move || !request || !gameplay || !actions || !observed.observed){policy.Reset();pendingStop=false;return single(a,b,c);}
    auto decision=TakeStop(config->Snapshot(),observed.frame);
    return RunSingle(single,decision,reinterpret_cast<void*>(move),reinterpret_cast<void*>(request),
        reinterpret_cast<void*>(gameplay),reinterpret_cast<void*>(actions),a,b,c);
}
}
bool InstallNative(Profile const& p,ConfigState& state,HookApi const& api,std::string& error) noexcept {
    try {
        if(active.load()){error="Already installed";return false;}
        if(!ValidateHost(p,base,error)){status.store(NativeStatus::Inactive);return false;}
        config=&state;
        HookSpec specs[]{{reinterpret_cast<void*>(base+0x6693b20),reinterpret_cast<void*>(InputDispatchHook),reinterpret_cast<void**>(&inputDispatch)},
                         {reinterpret_cast<void*>(base+0x6688630),reinterpret_cast<void*>(RawInputHook),reinterpret_cast<void**>(&rawInput)},
                         {reinterpret_cast<void*>(base+0xb1a5490),reinterpret_cast<void*>(BulkHook),reinterpret_cast<void**>(&bulk)},
                         {reinterpret_cast<void*>(base+0xb1a8140),reinterpret_cast<void*>(SingleHook),reinterpret_cast<void**>(&single)}};
        if(!hooks.Install(api,specs,error)){status.store(NativeStatus::Failed);return false;}
        active.store(true);status.store(NativeStatus::Ready);return true;
    }catch(...){hooks.Remove();status.store(NativeStatus::Failed);error="Native startup failed";return false;}
}
void RemoveNative() noexcept {
    // A callback can be suspended before taking our lock. Disable new detours,
    // but retain trampolines until process exit (the supported DLL is pinned).
    // Transactional startup rollback can still remove never-active hooks.
    active.store(false);hooks.Disable();AcquireSRWLockExclusive(&stateLock);policy.Reset();pendingStop=false;ReleaseSRWLockExclusive(&stateLock);
    status.store(NativeStatus::Inactive);
}
NativeStatus GetNativeStatus() noexcept {
    auto s=status.load();return s==NativeStatus::Ready && config && config->Snapshot().mode==SprintMode::Off?NativeStatus::Disabled:s;
}
InputTrace ReadInputTrace() noexcept {return {dispatchCount.load(),rawCount.load(),admittedCount.load(),requestCount.load(),jumpCount.load(),stopCount.load()};}
}
