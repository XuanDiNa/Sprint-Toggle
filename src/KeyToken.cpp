#include "ToggleSprint/KeyToken.h"
#include "../include/sdk/ModMenuBinding.h"
#include <array>
#include <charconv>
namespace sprint {
namespace {
struct KeyName { std::string_view name; std::uint16_t vk; };
constexpr KeyName names[]{
    {"NONE",0},{"LCTRL",162},{"RCTRL",163},{"LSHIFT",160},{"RSHIFT",161},{"LALT",164},{"RALT",165},
    {"SPACE",32},{"TAB",9},{"ENTER",13},{"INSERT",45},{"DELETE",46},{"HOME",36},{"END",35},
    {"PAGEUP",33},{"PAGEDOWN",34},{"UP",38},{"DOWN",40},{"LEFT",37},{"RIGHT",39},{"PAUSE",19},
    {";",186},{"=",187},{",",188},{"-",189},{".",190},{"/",191},{"`",192},{"[",219},{"\\",220},{"]",221},{"'",222}};
}
std::optional<std::uint16_t> ParseKeyToken(std::string_view text) noexcept {
    if(auto b=modmenu::binding::ParseInput(text))return static_cast<std::uint16_t>(b->key);
    for(auto [name,vk]:names) if(text==name) return vk;
    if(text.size()==1 && ((text[0]>='A' && text[0]<='Z') || (text[0]>='0' && text[0]<='9')))
        return static_cast<std::uint16_t>(text[0]);
    if(text.size()>=2 && text.size()<=3 && text[0]=='F' && text[1]!='0') {
        unsigned n{};auto r=std::from_chars(text.data()+1,text.data()+text.size(),n);
        if(r.ec==std::errc{} && r.ptr==text.data()+text.size() && n>=1 && n<=24) return static_cast<std::uint16_t>(111+n);
    }
    return {};
}
std::vector<std::string> SupportedKeys() {
    std::vector<std::string> r;
    for(auto [name,vk]:names) { (void)vk;r.emplace_back(name); }
    for(char c='A';c<='Z';++c)r.emplace_back(1,c);
    for(char c='0';c<='9';++c)r.emplace_back(1,c);
    for(int n=1;n<=24;++n)r.push_back("F"+std::to_string(n));
    return r;
}
}
