#include "Test.h"
#include "ModMenu/Settings.h"
#include "ToggleSprint/Config.h"
#include <windows.h>
#include <fstream>
#include <cstdio>
namespace fs=std::filesystem;
std::string Read(fs::path const& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
    try {
    using namespace menu;
    // Breaks caught: unsupported key declarations, wrong pointers, non-atomic Apply, Cancel writes.
    auto root=fs::temp_directory_path()/("ToggleSprintMenu-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    fs::create_directory(root);fs::create_directory(root/"ToggleSprint");auto dir=root/"ToggleSprint";
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto source=fs::path(exe).parent_path();
    for(unsigned n=0;n<8 && !fs::is_regular_file(source/"modmenu.settings.json");++n)source=source.parent_path();
    for(auto n:{"manifest.json","config.json","modmenu.settings.json"})fs::copy_file(source/n,dir/n,fs::copy_options::overwrite_existing);
    auto original=Read(dir/"config.json");auto draft=LoadSettings(root,"ToggleSprint");
    Check(draft.valid && draft.external && draft.hotReload && draft.fields.size()==2,"actual sidecar loads in real ModMenu parser");
    if(draft.valid && draft.fields.size()==2){
        for(auto token:{"LALT+K","CTRL+LALT+K","SHIFT+LALT+K","CTRL+SHIFT+LALT+K","CTRL+K","SHIFT+F24","CTRL+SHIFT+OEM_PLUS","BACKSPACE"}){
            auto edit=LoadSettings(root,"ToggleSprint");auto binding=modmenu::binding::Parse(token);
            Check(binding && SetBinding(edit.fields[1],*binding),"public binding edit");
            Check(ApplySettings(root,edit).saved,"chord transaction saves");std::string error;
            auto cfg=sprint::ParseConfig(Read(dir/"config.json"),error);
            Check(cfg && cfg->toggleVk==binding->key && cfg->toggleModifiers==binding->modifiers,"real runtime retains exact primary and modifiers");
        }
        std::ofstream(dir/"config.json",std::ios::binary)<<original;
        draft.fields[0].value=2;Check(SetKey(draft.fields[1],117),"F6 key capture");
        Check(Read(dir/"config.json")==original,"Cancel draft does not change file");
        draft=LoadSettings(root,"ToggleSprint");Check(std::get<int>(draft.fields[0].value)==1,"Cancel reopens original mode");
        for(int mode=0;mode<3;++mode){
            draft=LoadSettings(root,"ToggleSprint");draft.fields[0].value=mode;
            Check(SetKey(draft.fields[1],162),"left Ctrl accepted");
            auto result=ApplySettings(root,draft);std::string error;
            auto cfg=sprint::ParseConfig(Read(dir/"config.json"),error);
            Check(result.saved && cfg && static_cast<int>(cfg->mode)==mode && cfg->toggleToken=="LCTRL","three choice indices round trip to runtime");
        }
        draft=LoadSettings(root,"ToggleSprint");Check(SetKey(draft.fields[1],27),"Escape capture accepted");
        Check(ApplySettings(root,draft).saved,"unbound Apply saves");
        std::string error;auto cfg=sprint::ParseConfig(Read(dir/"config.json"),error);
        Check(cfg && cfg->toggleVk==0 && cfg->toggleToken=="NONE","Escape is real NONE, not default fallback");
        draft=LoadSettings(root,"ToggleSprint");Check(SetKey(draft.fields[1],8),"Backspace is a bindable primary");
        Check(ApplySettings(root,draft).saved,"Backspace Apply saves");
        cfg=sprint::ParseConfig(Read(dir/"config.json"),error);
        Check(cfg && cfg->toggleVk==8 && cfg->toggleModifiers==0 && cfg->toggleToken=="BACKSPACE","Backspace remains bound through Apply and runtime parsing");
    }
    for(auto const& p:fs::directory_iterator(dir))fs::remove(p.path());fs::remove(dir);fs::remove(root);
    return Finish("ModMenu");
    }catch(std::exception const& e){std::printf("ModMenu fixture error: %s\n",e.what());return 1;}
}
