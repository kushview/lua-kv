/// A Document Window.
// A window with a title bar and optional buttons. Is a @{kv.Widget}
// Backed by a JUCE DocumentWindow
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

    /// Close button pressed.
    // Called when the title bar close button is pressed.
    // @function DocumentWindow:closepressed
    // @within Handlers
    void closeButtonPressed() override
    {
        if (sol::safe_function f = widget ["closepressed"])
            f (widget);
    }

    void setContent (const sol::object& child)
    {
        switch (child.get_type())
        {
            case sol::type::table:
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
                break;
            }
            
            case sol::type::nil:
            {
                clearContentComponent();
                content = sol::lua_nil;
                break;
            }

            default:
                break;
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
        "addtodesktop",   [](DocumentWindow& self) { self.addToDesktop(); },
        "setcontent",      &DocumentWindow::setContent,
        "getcontent",      &DocumentWindow::getContent,
        "content", sol::property (&DocumentWindow::getContent, 
                                  &DocumentWindow::setContent),
        
        sol::base_classes, sol::bases<juce::DocumentWindow,
                                      juce::ResizableWindow,
                                      juce::TopLevelWindow,
                                      juce::Component,
                                      juce::MouseListener>()
    );

    auto T_mt = T[sol::metatable_key];

    /// Attributes.
    // @section attributes
    sol::table props = T_mt["__props"];
    props.add (
        /// Displayed content.
        // Assign to the widget you wish to display. Set to nil to clear the
        // content.
        // @tfield kv.Widget DocumentWindow.content
        "content"
    );
    
    /// Methods.
    // @section methods
    sol::table methods = T_mt["__methods"];
    methods.add (
        "getcontent",
        "setcontent"
    );

    sol::stack::push (L, T);
    return 1;
}
