/// A JUCE DocumentWindow usertype
// @classmod kv.DocumentWindow
// @pragma nostrip

#include "kv/lua/object.hpp"
#include "kv/lua/widget.hpp"
#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_WINDOW     "DocumentWindow"

using namespace juce;

namespace kv {
namespace lua {

class DocumentWindow : public juce::DocumentWindow
{
public:
    DocumentWindow (const sol::table&)
        : juce::DocumentWindow ("", Colours::black, DocumentWindow::allButtons, true)
    { }

    ~DocumentWindow() override
    {
        widget = sol::lua_nil;
    }

    static void init (const sol::table& proxy) {
        if (auto* const impl = object_userdata<DocumentWindow> (proxy)) {
            impl->widget = proxy;
            impl->setUsingNativeTitleBar (true);
            impl->setResizable (true, false);
        }
    }

    void resized() override
    {
        juce::DocumentWindow::resized();
    }

    void closeButtonPressed() override
    {
        if (sol::safe_function f = widget ["onclosebutton"])
            f (widget);
    }

    void setContent (const sol::table& child)
    {
        if (Component* const comp = object_userdata<Component> (child))
        {
            content = child;
            setContentNonOwned (comp, true);
        }
        else
        {
            // DBG("failed to set widget");
        }
    }

    sol::table getContent() const { return content; }

private:
    sol::table widget;
    sol::table content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DocumentWindow)
};

}}

LUAMOD_API
int luaopen_kv_DocumentWindow (lua_State* L) {
    using kv::lua::DocumentWindow;

    auto T = kv::lua::new_widgettype<DocumentWindow> (L, LKV_TYPE_NAME_WINDOW,
        sol::meta_method::to_string, [](DocumentWindow& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_WINDOW);
        },
        "add_to_desktop",   [](DocumentWindow& self) { self.addToDesktop(); },
        "set_content",      &DocumentWindow::setContent,
        "get_content",      &DocumentWindow::getContent,
        "content", sol::property (&DocumentWindow::getContent, &DocumentWindow::setContent),
        
        sol::base_classes, sol::bases<juce::DocumentWindow,
                                      juce::ResizableWindow,
                                      juce::TopLevelWindow,
                                      juce::Component,
                                      juce::MouseListener>()
    );

    auto T_mt = T[sol::metatable_key];
    ((sol::table) T_mt["__props"]).add ("content");
    ((sol::table) T_mt["__methods"]).add ("get_content", "set_content");

    sol::stack::push (L, T);
    return 1;
}
