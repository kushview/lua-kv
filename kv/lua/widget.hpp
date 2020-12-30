
#pragma once
#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename WidgetType, typename ...Args>
inline static sol::table
new_widgettype (lua_State* L, const char* name, Args&& ...args) {
    using Widget = WidgetType;
    sol::state_view lua (L);
    sol::table M = lua.create_table();

    M.new_usertype<Widget> (name, sol::no_constructor,
        "init", Widget::init,

        "name", sol::property (
            [](Widget& self) { return self.getName().toStdString(); },
            [](Widget& self, const char* name) { self.setName (name); }
        ),

        "localbounds",          sol::readonly_property (&Widget::getLocalBounds),
        
        "bounds",               sol::property (
            &Widget::getBounds,
            [](Widget& self, const sol::object obj) {
                if (obj.is<Rectangle<int>>())
                    self.setBounds (obj.as<Rectangle<int>>());
            }
        ),

        "x",                    sol::readonly_property (&Widget::getX),
        "y",                    sol::readonly_property (&Widget::getY),
        "width",                sol::readonly_property (&Widget::getWidth),
        "height",               sol::readonly_property (&Widget::getHeight),
        "right",                sol::readonly_property (&Widget::getRight),
        "bottom",               sol::readonly_property (&Widget::getBottom),

        "screenx",              sol::readonly_property (&Widget::getScreenX),
        "screeny",              sol::readonly_property (&Widget::getScreenY),

        "visible",              sol::property (&Widget::setVisible, &Widget::isVisible),

        "repaint",  sol::overload (
            [](Widget& self) { self.repaint(); },
            [](Widget& self, const Rectangle<int>& r) { 
                self.repaint (r); 
            },
            [](Widget& self, const Rectangle<double>& r) { 
                self.repaint (r.toNearestInt());
            },
            [](Widget& self, int x, int y, int w, int h) { 
                self.repaint (x, y, w, h); 
            }
        ),

        "resize",               &Widget::setSize,
        "tofront",              &Widget::toFront,
        "toback",               &Widget::toBack,

        "remove_from_desktop",  &Widget::removeFromDesktop,
        "is_on_desktop",        &Widget::isOnDesktop,
        
        "set_bounds", [](Widget& self, double x, double y, double w, double h) {
            self.setBounds (x, y, w, h);
        },
        
        std::forward <Args> (args)...
        // sol::base_classes,      sol::bases<juce::Component>()
    );

    auto T = kv::lua::remove_and_clear (M, name);
    auto T_mt = T[sol::metatable_key];
    T_mt["__newindex"] = sol::lua_nil;
    T_mt["__newuserdata"] = [L]() {
        sol::state_view view (L);
        return std::make_unique<Widget> (view.create_table());
    };

    T_mt["__props"] = lua.create_table().add (
        "name",
        "localbounds", "bounds",
        "x", "y", "width", "height", "right", "bottom",
        "screenx", "screeny",
        "visible"
    );
    
    T_mt["__methods"] = lua.create_table().add (
        "repaint",
        "resize",
        "tofront",
        "toback",

        "get_bounds",
        "set_bounds",

        "is_on_desktop", 
        "add_to_desktop",
        "remove_from_desktop"
    );

    lua.script (R"(
        require ('kv.Bounds')
        require ('kv.Graphics')
        require ('kv.Point')
        require ('kv.Rectangle')
    )");

    return T;
}

template<typename SliderType, typename ...Args>
inline static sol::table
new_slidertype (lua_State* L, const char* name, Args&& ...args) {
    return new_widgettype<SliderType> (name,
    std::forward<Args> (args)...);
}

}}
