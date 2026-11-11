#include "Test.h"
#include "ToggleSprint/KeyToken.h"
using namespace sprint;
int main() {
    Check(ParseKeyToken("LCTRL")==0xA2,"left ctrl is distinct from right ctrl");
    Check(ParseKeyToken("RCTRL")==0xA3,"right ctrl");
    Check(ParseKeyToken("NONE")==0,"NONE never falls back to ctrl");
    Check(ParseKeyToken("A")==65 && ParseKeyToken("0")==48,"letters and digit VK");
    Check(ParseKeyToken("F1")==112 && ParseKeyToken("F24")==135,"function endpoints");
    Check(ParseKeyToken("PAGEUP")==33 && ParseKeyToken("SPACE")==32,"navigation and space");
    Check(ParseKeyToken("LSHIFT")==160 && ParseKeyToken("RALT")==165,"modifier sides");
    Check(ParseKeyToken(";")==186 && ParseKeyToken("/")==191,"punctuation");
    Check(ParseKeyToken("CTRL+A")==65&&ParseKeyToken("BACKSPACE")==8,"public chords and editing key supported");
    for (auto s:{"F0","F25","F01","MOUSE1","ESCAPE","","UNKNOWN"})
        Check(!ParseKeyToken(s),"reject noncanonical or unavailable token");
    for (auto const& token:SupportedKeys()) Check(ParseKeyToken(token).has_value(),"advertised key parses");
    return Finish("KeyToken");
}
