#include "ToggleSprint/ConfigFile.h"
#include "ToggleSprint/NativeInput.h"
#include <windows.h>
#include <MinHook.h>
#include <atomic>
#include <filesystem>
#include <fstream>
namespace {
using namespace sprint;
HMODULE module{};HANDLE stopEvent{};std::atomic<int> startup{-1};
ConfigState config;std::filesystem::path directory;
void Log(std::string const& message) noexcept {
    try {
        auto line="[ToggleSprint] "+message+"\n";OutputDebugStringA(line.c_str());
        if(!directory.empty())std::ofstream(directory/L"ToggleSprint.log",std::ios::app|std::ios::binary)<<GetTickCount64()<<' '<<line;
    }catch(...){}
}
int Initialize(){return static_cast<int>(MH_Initialize());}
int Create(void* target,void* replacement,void** original){return static_cast<int>(MH_CreateHook(target,replacement,original));}
int Enable(void* target){return static_cast<int>(MH_EnableHook(target));}
int Disable(void* target){return static_cast<int>(MH_DisableHook(target));}
int Remove(void* target){return static_cast<int>(MH_RemoveHook(target));}
int Uninitialize(){return static_cast<int>(MH_Uninitialize());}
DWORD WINAPI Worker(void* reference){
    try {
        wchar_t path[32768]{};
        if(!GetModuleFileNameW(module,path,32768))throw std::runtime_error("Cannot resolve plugin folder");
        directory=std::filesystem::path(path).parent_path();Log("START 0.1.0-test7-leftalt; exact 1.26.52.03; Windows x64");
        std::string error;std::uintptr_t host{};
        if(!ValidateHost(ExactProfile(),host,error)){
            Log("INACTIVE: "+error);startup.store(0);FreeLibraryAndExitThread(static_cast<HMODULE>(reference),0);
        }
        ConfigFile file(directory/L"config.json");
        if(file.Poll(config,error)==ReloadResult::Rejected)Log("CONFIG REFUSED: "+error+"; packaged defaults retained");
        // Native hooks can be executing during launcher state changes. Pin only
        // a supported host, before enabling hooks; live DLL unloading is unsupported.
        HMODULE pinned{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&Worker),&pinned))throw std::runtime_error("Cannot pin active module");
        HookApi api{Initialize,Create,Enable,Disable,Remove,Uninitialize};
        if(!InstallNative(ExactProfile(),config,api,error)){
            Log("INACTIVE: "+error);startup.store(static_cast<int>(GetNativeStatus()));
        }else{
            Log("READY profile=1.26.52.03 mode="+std::to_string(static_cast<unsigned>(config.Snapshot().mode))+" key="+config.Snapshot().toggleToken);
            startup.store(1);std::string lastError;unsigned tracePoll{};std::uint64_t lastAdmitted{};
            while(WaitForSingleObject(stopEvent,500)==WAIT_TIMEOUT){
                auto result=file.Poll(config,error);
                if(result==ReloadResult::Applied){
                    auto cfg=config.Snapshot();lastError.clear();Log("CONFIG APPLIED mode="+std::to_string(static_cast<unsigned>(cfg.mode))+" key="+cfg.toggleToken);
                }else if(result==ReloadResult::Rejected && error!=lastError){lastError=error;Log("CONFIG REFUSED: "+error);}
                // Bounded test-build route counters, never per-tick logging.
                if(++tracePoll%10==0){
                    auto t=ReadInputTrace();
                    if(tracePoll==10 || t.admitted!=lastAdmitted){
                        Log("INPUT dispatch="+std::to_string(t.dispatch)+" raw="+std::to_string(t.raw)+" local="+std::to_string(t.admitted)+
                            " requested="+std::to_string(t.requested)+" jump="+std::to_string(t.jump)+" stops="+std::to_string(t.stops));
                        lastAdmitted=t.admitted;
                    }
                }
            }
            RemoveNative();startup.store(0);Log("STOPPED");
        }
    }catch(std::exception const& e){RemoveNative();Log(std::string("FAILED: ")+e.what());startup.store(3);}
    catch(...){RemoveNative();Log("FAILED: startup exception");startup.store(3);}
    FreeLibraryAndExitThread(static_cast<HMODULE>(reference),0);
}
}
extern "C" __declspec(dllexport) int __cdecl TS_GetStatus(){return startup.load()==1?static_cast<int>(sprint::GetNativeStatus()):startup.load();}
extern "C" __declspec(dllexport) char const* __cdecl TS_GetVersion(){return "0.1.0-test7-leftalt";}
extern "C" __declspec(dllexport) void __cdecl TS_Shutdown(){if(stopEvent)SetEvent(stopEvent);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved){
    if(reason==DLL_PROCESS_ATTACH){
        module=instance;stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        HMODULE reference{};
        if(!stopEvent || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&Worker),&reference)){startup.store(3);return TRUE;}
        auto thread=CreateThread(nullptr,0,Worker,reference,0,nullptr);
        if(thread)CloseHandle(thread);else{startup.store(3);FreeLibrary(reference);}
    }else if(reason==DLL_PROCESS_DETACH){
        // Never wait, acquire native locks or tear down hooks under loader lock.
        // Successful game hosts are pinned. At process exit the OS owns teardown.
        if(!reserved && stopEvent){CloseHandle(stopEvent);stopEvent=nullptr;}
    }
    return TRUE;
}
