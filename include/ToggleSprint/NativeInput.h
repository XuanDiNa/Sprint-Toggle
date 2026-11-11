#pragma once
#include "Config.h"
#include "SprintPolicy.h"
#include "Profile.h"
#include <span>
namespace sprint {
enum class NativeStatus:int { Inactive, Ready, Disabled, Failed };
using BulkFn=std::uintptr_t(__fastcall*)(void*,std::uint32_t*,void*,void*,void*,void*,void*,void*,void*,void*,void*);
using SingleFn=std::uintptr_t(__fastcall*)(void*,void*,std::uint32_t*);
using InputDispatchFn=std::uintptr_t(__fastcall*)(void*,void*);
using RawInputFn=std::uintptr_t(__fastcall*)(void*,void*,void*,void*,void*,void*);
struct HookApi {
    int(*initialize)();int(*create)(void*,void*,void**);int(*enable)(void*);
    int(*disable)(void*);int(*remove)(void*);int(*uninitialize)();
};
struct HookSpec {void* target;void* replacement;void** original;};
class HookSet {
    HookApi api_{};std::vector<void*> created_,enabled_;bool initialized_{},ownsInitialization_{},everEnabled_{};
public:
    bool Install(HookApi const&,std::span<HookSpec const>,std::string&);
    void Disable() noexcept;
    void Remove() noexcept;
};
// Post-copy raw input (+0 bit 0x100), before ordinary movement/request processing.
// Never change physical MoveInput+16 or manufacture the derived +96 sprint flag.
std::uintptr_t RunRawInput(RawInputFn,SprintPolicy&,Config const&,SprintFrame,SprintDecision&,
    void*,void*,void*,void*,void*,void*);
// Consumers preserve all input; only a fresh queued unlatch emits vanilla stop outputs.
std::uintptr_t RunBulk(BulkFn,SprintDecision const&,
    void*,std::uint32_t*,void*,void*,void*,void*,void*,void*,void*,void*,void*);
std::uintptr_t RunSingle(SingleFn,SprintDecision const&,
    void* move,void* request,void* gameplay,void* actions,void*,void*,std::uint32_t*);
SprintDecision ConsumeStop(SprintPolicy&,bool& pending,std::uint64_t decisionRevision,Config const&,SprintFrame const&);
struct InputTrace {std::uint64_t dispatch{},raw{},admitted{},requested{},jump{},stops{};};
InputTrace ReadInputTrace() noexcept;
std::uintptr_t FindPool(std::uintptr_t registry,std::uint32_t hash) noexcept;
bool PoolContains(std::uintptr_t pool,std::uint32_t entity) noexcept;
std::uintptr_t Component(std::uintptr_t pool,std::uint32_t entity,unsigned stride) noexcept;
bool MatchRawInput(std::uintptr_t root,void* move,void* raw,void* entityRef,std::uint32_t& entity) noexcept;
bool InstallNative(Profile const&,ConfigState&,HookApi const&,std::string&) noexcept;
void RemoveNative() noexcept;
NativeStatus GetNativeStatus() noexcept;
}
