/// A JUCE DocumentWindow usertype
// @classmod kv.DocumentWindow
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_WINDOW     "DocumentWindow"

using namespace juce;

namespace kv {
namespace lua {

class DocumentWindowImpl : public DocumentWindow
{
public:
    ~DocumentWindowImpl()
    {
        widget = sol::lua_nil;
    }

    static DocumentWindowImpl* create (sol::table tbl)
    {
        auto* wrapper = new DocumentWindowImpl();
        wrapper->widget = tbl;
        return wrapper;
    }

    void closeButtonPressed() override
    {
        if (sol::safe_function f = widget ["onclosebutton"])
            f (widget);
    }

    void set_widget (sol::table child)
    {
        if (auto* const comp = child.get<Component*> ("impl"))
        {
            setContentNonOwned (comp, true);
        }
        else
        {
            // DBG("failed to set widget");
        }
    }

private:
    explicit DocumentWindowImpl (const String& name = "Window")
        : DocumentWindow (name, Colours::darkgrey, DocumentWindow::allButtons, false)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
    }

    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DocumentWindowImpl)
};

}}

LUAMOD_API
int luaopen_kv_DocumentWindow (lua_State* L) {
    using kv::lua::DocumentWindowImpl;
    sol::state_view lua (L);
    auto t = lua.create_table();
    t.new_usertype<DocumentWindowImpl> (LKV_TYPE_NAME_WINDOW, sol::no_constructor,
        sol::call_constructor,  sol::factories (DocumentWindowImpl::create),
        sol::meta_method::to_string, [](DocumentWindowImpl& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_WINDOW);
        },
        "get_name",             [](DocumentWindowImpl& self) { return self.getName().toStdString(); },
        "set_name",             [](DocumentWindowImpl& self, const char* name) { self.setName (name); },
        "set_size",             &DocumentWindowImpl::setSize,
        "set_visible",          &DocumentWindowImpl::setVisible,
        "repaint",              [](DocumentWindowImpl& self) { self.repaint(); },
        "is_visible",           &DocumentWindowImpl::isVisible,
        "get_width",            &DocumentWindowImpl::getWidth,
        "get_height",           &DocumentWindowImpl::getHeight,
        "add_to_desktop",       [](DocumentWindowImpl& self) { self.addToDesktop(); },
        "remove_from_desktop",  &DocumentWindowImpl::removeFromDesktop,
        "is_on_desktop",        &DocumentWindowImpl::isOnDesktop,
        "set_content_owned",    &DocumentWindowImpl::setContentOwned,
        "set_widget",           &DocumentWindowImpl::set_widget,
        sol::base_classes,      sol::bases<juce::DocumentWindow, juce::Component, juce::MouseListener>()
    );

    sol::stack::push (L, kv::lua::remove_and_clear (t, LKV_TYPE_NAME_WINDOW));
    return 1;
}
