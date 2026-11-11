#include "Test.h"
#include <windows.h>
#include <string>
#include <filesystem>
int wmain(int argc,wchar_t** argv){
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
    auto dll=argc==2?std::filesystem::path(argv[1]):std::filesystem::path(exe).parent_path()/L"ToggleSprint.dll";
    std::filesystem::path isolated;
    if(argc==2){
        // Worker diagnostics go beside the loaded DLL, so never load a frozen
        // payload in place. Keep all test writes in a new isolated directory.
        isolated=std::filesystem::temp_directory_path()/(L"ToggleSprintSmoke-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
        std::filesystem::create_directory(isolated);
        std::filesystem::copy_file(dll,isolated/L"ToggleSprint.dll");dll=isolated/L"ToggleSprint.dll";
    }
    auto module=LoadLibraryW(dll.c_str());
    Check(module!=nullptr,"DLL loads in unsupported process");
    if(module){
        auto status=reinterpret_cast<int(*)()>(GetProcAddress(module,"TS_GetStatus"));
        auto version=reinterpret_cast<char const*(*)()>(GetProcAddress(module,"TS_GetVersion"));
        auto stop=reinterpret_cast<void(*)()>(GetProcAddress(module,"TS_Shutdown"));
        Check(status && version && stop,"bounded diagnostic exports");
        if(status && version && stop){
            for(unsigned n=0;n<100 && status()==-1;++n)Sleep(20);
            Check(status()==0,"unsupported host never hooks");
            Check(std::string(version())=="0.1.0-test7-leftalt","exact binary version");
            stop();stop();Check(status()==0,"repeated shutdown inactive");
        }
        FreeLibrary(module);
    }
    if(!isolated.empty()){
        for(unsigned n=0;n<100 && GetModuleHandleW(dll.c_str());++n)Sleep(20);
        std::error_code error;
        Check(std::filesystem::remove(dll,error) && !error,"isolated DLL unloaded and removed");
        std::filesystem::remove(isolated/L"ToggleSprint.log",error);
        Check(!error,"isolated diagnostics removed");
        Check(std::filesystem::remove(isolated,error) && !error,"isolated smoke directory removed");
    }
    return Finish("Smoke");
}
