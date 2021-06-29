/// A Text Button.
// Is a @{kv.Widget}.
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

    /// On clicked handler.
    // Executed when the button is clicked by the user.
    // @function TextButton:clicked
    void buttonClicked (Button*) override {
        try {
            if (sol::protected_function f = widget ["clicked"])
                f (widget);
        } catch (const sol::error& e) {
            std::cerr << e.what() << std::endl;
        }
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
        "togglestate", sol::property (
            [](TextButton& self, bool state) {
                self.setToggleState (state, sendNotification);
            },
            [](TextButton& self) {
                return self.getToggleState();
            }
        ),
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

    /// Attributes.
    // @section attributes
    __props.add (
        /// Displayed text.
        // @tfield string TextButton.text
        "text",

        /// The button's toggle state.
        // Setting this property will notify listeners.
        // @tfield bool TextButton.togglestate
        "togglestate"
    );

    sol::stack::push (L, T);
    return 1;
}
