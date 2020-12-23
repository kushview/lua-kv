/// A JUCE Component userdata
// @classmod kv.TextButton
// @pragma nostrip

#include "kv/lua/object.hpp"
#include "kv/lua/widget.hpp"
#define LKV_TYPE_NAME_TEXT_BUTTON     "TextButton"

using namespace juce;

namespace kv {
namespace lua {

class TextButton : public juce::TextButton,
                   public Button::Listener
{
public:
    TextButton (const sol::table& obj) {
        addListener (this);
    }
    ~TextButton() {
        removeListener (this);
    }


    static void init (const sol::table& proxy) {
        if (auto* const impl = object_userdata<TextButton> (proxy))
            impl->widget = proxy;
    }

    void buttonClicked (Button*) override {
        if (sol::function f = widget ["onclick"])
            f (widget);
    }

private:
    TextButton() = delete;
    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextButton)
};

}}

LUAMOD_API
int luaopen_kv_TextButton (lua_State* L) {
    using kv::lua::TextButton;

    auto T = kv::lua::new_widgettype<TextButton> (L, LKV_TYPE_NAME_TEXT_BUTTON,
        sol::meta_method::to_string, [](TextButton& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_TEXT_BUTTON);
        },
        "text", sol::property (
            [](TextButton& self, const char* text) {
                self.setButtonText (String::fromUTF8 (text));
            },
            [](TextButton& self) {
                return self.getButtonText().toStdString();
            }
        ),
        sol::base_classes, sol::bases<Component>()
    );

    auto T_mt = T[sol::metatable_key];
    sol::table __props = T_mt["__props"];
    __props.add ("text");

    sol::stack::push (L, T);
    return 1;
}
