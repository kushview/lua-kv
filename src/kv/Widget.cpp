/// A GUI Widget.
// Is defined with @{kv.object} and can be inherrited. Backed by a JUCE Component.
// @classmod kv.Widget
// @pragma nostrip

#include "kv/lua/widget.hpp"
#define LKV_TYPE_NAME_WIDGET     "Widget"

using namespace juce;

namespace kv {
namespace lua {

class Widget : public juce::Component
{
public:
    ~Widget()
    {
        Desktop::getInstance().getGlobalScaleFactor();
        widget = sol::lua_nil;
    }

    Widget (const sol::table& obj)
    {
        widget = obj;
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
        if (sol::safe_function f = widget ["mousedrag"])
            f (widget, ev);
    }

    void mouseDown (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mousedown"])
            f (widget, ev);
    }

    void mouseUp (const MouseEvent& ev) override
    {
        if (sol::safe_function f = widget ["mouseup"])
            f (widget, ev);
    }

    template<typename T>
    static T* userdata (const sol::table& proxy) {
        if (! proxy.valid())
            return nullptr;
        auto mt = proxy[sol::metatable_key];
        return mt["__impl"].get_or<T*> (nullptr);
    }

    sol::table addWithZ (const sol::object& child, int zorder)
    {
        jassert (child.valid());
        if (Component* const impl = userdata<Component> (child))
        {
            addAndMakeVisible (*impl, zorder);
        }

        return child;
    }

    sol::table add (const sol::object& child)
    {
        return addWithZ (child, -1);
    }
    
    static void init (const sol::table& proxy) {
        if (auto* const impl = userdata<Widget> (proxy)) {
            impl->widget = proxy;
        }
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
    Widget() = delete;
    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Widget)
};

}}

LUAMOD_API
int luaopen_kv_Widget (lua_State* L) {
    using kv::lua::Widget;

    auto T = kv::lua::new_widgettype<Widget> (L, LKV_TYPE_NAME_WIDGET,
        sol::meta_method::to_string, [](Widget& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_WIDGET);
        },
        "add", sol::overload (&Widget::add, &Widget::addWithZ),
        "addtodesktop", sol::overload (
            [](Widget& self, int flags) { 
                self.addToDesktop(flags, nullptr); 
            },
            [](Widget& self, int flags, void* handle) { 
                self.addToDesktop (flags, handle); 
            }
        ),
        sol::base_classes,      sol::bases<Component>()
    );

    sol::table T_mt = T [sol::metatable_key];
    T_mt["__methods"].get_or_create<sol::table>().add (
        /// Add a child widget.
        // @function Widget:add
        // @tparam kv.Widget widget Widget to add
        // @int[opt] zorder Z-order
        // @within Methods
        "add"
    );

    sol::stack::push (L, T);

    return 1;
}
