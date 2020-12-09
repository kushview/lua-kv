/// A JUCE Component userdata
// @classmod kv.Component
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER
#define LKV_TYPE_NAME_COMPONENT     "Component"

using namespace juce;

namespace kv {
namespace lua {

class ComponentImpl : public juce::Component
{
public:
    ~ComponentImpl()
    {
        widget = sol::lua_nil;
    }

    static ComponentImpl* create (sol::table obj)
    {
        auto* wrapper = new ComponentImpl();
        wrapper->widget = obj;
        wrapper->resized();
        wrapper->repaint();
        return wrapper;
    }

    void resized() override
    {
        if (sol::safe_function f = widget ["resized"])
            f (widget);
    }

    void paint (Graphics& g) override
    {
        if (sol::safe_function f = widget ["paint"]) {
            f (widget, std::ref<Graphics> (g));
        }
    }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_drag"])
            f (widget, ev);
    }

    void mouseDown (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_down"])
            f (widget, ev);
    }

    void mouseUp (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouse_up"])
            f (widget, ev);
    }

    void add (sol::table child, int zorder)
    {
        if (Component* const impl = child.get<Component*> ("impl"))
            addAndMakeVisible (*impl, zorder);
    }
    
    sol::table getBoundsTable()
    {
        sol::state_view L (widget.lua_state());
        auto r = getBounds();
        auto t = L.create_table();
        t["x"]      = r.getX();
        t["y"]      = r.getY();
        t["width"]  = r.getWidth();
        t["height"] = r.getHeight();
        return t;
    }

private:
    ComponentImpl() = default;
    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentImpl)
};

}}

LUAMOD_API
int luaopen_kv_Component (lua_State* L) {
    using kv::lua::ComponentImpl;
    sol::state_view lua (L);

    auto M = lua.create_table();
    M.new_usertype<ComponentImpl> (LKV_TYPE_NAME_COMPONENT, sol::no_constructor,
        sol::call_constructor, sol::factories (ComponentImpl::create),

        sol::meta_method::to_string, [](ComponentImpl& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_COMPONENT);
        },

        "get_name",           [](ComponentImpl& self) { return self.getName().toStdString(); },
        "set_name",           [](ComponentImpl& self, const char* name) { self.setName (name); },
        "set_size",             &ComponentImpl::setSize,
        "set_visible",          &ComponentImpl::setVisible,
        "repaint",            [](ComponentImpl& self) { self.repaint(); },
        "is_visible",           &ComponentImpl::isVisible,
        "get_width",            &ComponentImpl::getWidth,
        "get_height",           &ComponentImpl::getHeight,
        "add_to_desktop",     [](ComponentImpl& self) { self.addToDesktop (0); },
        "remove_from_desktop",  &ComponentImpl::removeFromDesktop,
        "is_on_desktop",        &ComponentImpl::isOnDesktop,
        "add",                  &ComponentImpl::add,
        "resize",               &ComponentImpl::setSize,
        "set_bounds",         [](ComponentImpl& self, int x, int y, int w, int h) {
            self.setBounds (x, y, w, h);
        },
        "get_bounds",           &ComponentImpl::getBoundsTable,
        sol::base_classes,      sol::bases<juce::Component>()
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, LKV_TYPE_NAME_COMPONENT));
    return 1;
}
