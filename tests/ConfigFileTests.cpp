#include "Test.h"
#include "ToggleSprint/ConfigFile.h"
#include <windows.h>
#include <fstream>
using namespace sprint;
namespace {void Write(std::filesystem::path const& p,std::string const& s){std::ofstream(p,std::ios::binary)<<s;}}
int main(){
    // Breaks caught: reparsing unchanged bytes, torn replacement, resetting good state on invalid files.
    auto dir=std::filesystem::temp_directory_path()/("ToggleSprintConfig-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directory(dir);auto path=dir/"config.json",replacement=dir/"next.json";
    ConfigState state;ConfigFile file(path);std::string error;
    Check(file.Poll(state,error)==ReloadResult::Rejected,"missing file reports refusal");
    Write(path,R"({"mode":2,"toggleKey":"NONE"})");
    Check(file.Poll(state,error)==ReloadResult::Applied && state.Snapshot().mode==SprintMode::Force,"initial full snapshot applied");
    auto revision=state.Snapshot().revision;
    Check(file.Poll(state,error)==ReloadResult::Unchanged && revision==state.Snapshot().revision,"unchanged bytes not parsed twice");
    Write(replacement,R"({"mode":1,"toggleKey":"F6"})");
    Check(MoveFileExW(replacement.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,"real atomic replacement");
    Check(file.Poll(state,error)==ReloadResult::Applied && state.Snapshot().toggleVk==117 && state.Snapshot().revision==revision+1,"one changed complete snapshot");
    Write(path,R"({"mode":0,"toggleKey":)");
    Check(file.Poll(state,error)==ReloadResult::Rejected && state.Snapshot().mode==SprintMode::Toggle,"invalid replacement retains last good");
    Check(file.Poll(state,error)==ReloadResult::Unchanged,"same invalid payload not logged repeatedly");
    Write(path,std::string(65537,' '));
    Check(file.Poll(state,error)==ReloadResult::Rejected,"oversize rejected");
    Write(path,R"({"mode":0,"toggleKey":"LCTRL"})");
    Check(file.Poll(state,error)==ReloadResult::Applied && state.Snapshot().mode==SprintMode::Off,"recovery from invalid config");
    std::filesystem::remove(path);std::filesystem::remove(replacement);std::filesystem::remove(dir);
    return Finish("ConfigFile");
}
