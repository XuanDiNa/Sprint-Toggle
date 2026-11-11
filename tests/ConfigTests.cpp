#include "Test.h"
#include "ToggleSprint/Config.h"
using namespace sprint;
int main() {
    {std::string error;Check(sprint::ParseConfig(R"({"toggleKey":"CTRL+SHIFT+K"})",error).has_value(),"public chord accepted by real config parser");}
    std::string error;
    // Catches storing only the primary VK and silently losing modifier bits.
    struct KeyCase {char const* token;unsigned key,mask;};
    for(auto const& key:std::initializer_list<KeyCase>{
        {"K",75,0},{"CTRL+K",75,1},{"SHIFT+K",75,2},{"CTRL+SHIFT+K",75,3},
        {"LALT+K",75,4},{"CTRL+LALT+K",75,5},{"SHIFT+LALT+K",75,6},
        {"CTRL+SHIFT+LALT+K",75,7},{"CTRL+LALT",164,1},{"BACKSPACE",8,0},
        {"left_alt",164,0},{"unbound",0,0},{";",186,0},{"0x75",117,0}}){
        auto parsed=ParseConfig(std::string("{\"toggleKey\":\"")+key.token+"\"}",error);
        Check(parsed && parsed->toggleVk==key.key && parsed->toggleModifiers==key.mask,"full binding reaches runtime config");
        Check(parsed && parsed->toggleToken==key.token,"legacy input spelling retained without rewriting");
    }
    auto c=ParseConfig("{}",error);
    Check(c && c->mode==SprintMode::Toggle && c->toggleVk==0xA2,"default toggle left ctrl");
    for(int m=0;m<3;++m) Check(ParseConfig("{\"mode\":"+std::to_string(m)+"}",error).has_value(),"three mode values");
    c=ParseConfig(R"({"mode":2,"toggleKey":"NONE"})",error);
    Check(c && c->mode==SprintMode::Force && c->toggleVk==0,"NONE force config");
    for (auto s:{R"({"mode":true})",R"({"mode":1.0})",R"({"mode":-1})",R"({"mode":3})",
         R"({"mode":1,"mode":2})",R"({"extra":1})",R"({"toggleKey":42})",R"({"toggleKey":"CTRL+NONE"})",
         R"({"toggleKey":"F25"})","[]","null","{bad","{} {}"})
        Check(!ParseConfig(s,error) && !error.empty(),"invalid full candidate rejected");
    Check(!ParseConfig(std::string(65537,' '),error),"bounded config size");
    ConfigState state;
    Check(state.Apply(R"({"mode":2,"toggleKey":"F6"})",error),"valid publication");
    auto before=state.Snapshot();
    Check(!state.Apply(R"({"mode":0,"toggleKey":"invalid"})",error),"whole transaction rejected");
    auto after=state.Snapshot();
    Check(after.mode==SprintMode::Force && after.toggleVk==117 && before.revision==after.revision,"last valid snapshot retained");
    Check(state.Apply(R"({"mode":1,"toggleKey":"NONE"})",error),"explicit unbind");
    after=state.Snapshot();
    Check(after.toggleVk==0 && after.revision>before.revision,"unbind published with new generation");
    auto revision=after.revision;
    state.Apply(R"({"mode":1,"toggleKey":"NONE"})",error);
    Check(state.Snapshot().revision==revision,"same config does not reset latch");
    state.Apply(R"({"toggleKey":"CTRL+K"})",error);revision=state.Snapshot().revision;
    state.Apply(R"({"toggleKey":"SHIFT+K"})",error);
    Check(state.Snapshot().revision>revision && state.Snapshot().toggleModifiers==2,"modifier-only edit publishes a new config revision");
    revision=state.Snapshot().revision;
    Check(!state.Apply(R"({"toggleKey":"LALT+LALT"})",error) && state.Snapshot().revision==revision,"invalid chord retains last valid configuration");
    return Finish("Config");
}
