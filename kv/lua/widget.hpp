/// A GUI Widget.
// @classmod kv.Widget
// @pragma nostrip

#pragma once
#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename WidgetType>
static void widget_setbounds (WidgetType& self, const sol::object& obj) {
    if (obj.is<Rectangle<int>>())
    {
        self.setBounds (obj.as<Rectangle<int>>());
    }
    else if (obj.is<sol::table>())
    {
        sol::table tr = obj;
        self.setBounds (
            tr.get_or ("x",      self.getX()),
            tr.get_or ("y",      self.getY()),
            tr.get_or ("width",  self.getWidth()),
            tr.get_or ("height", self.getHeight())
        );
    }
}

template<typename WidgetType, typename ...Args>
inline static sol::table
new_widgettype (lua_State* L, const char* name, Args&& ...args) {
    using Widget = WidgetType;
    sol::state_view lua (L);
    sol::table M = lua.create_table();

    M.new_usertype<Widget> (name, sol::no_constructor,
        /// Initialize the widget.
        // Override this to customize your widget.
        // @function Widget.init
        "init", Widget::init,

        "name", sol::property (
            [](Widget& self) { return self.getName().toStdString(); },
            [](Widget& self, const char* name) { self.setName (name); }
        ),

        "localbounds",          sol::readonly_property (&Widget::getLocalBounds),
        
        "bounds",               sol::property (
            &Widget::getBounds,
            [](Widget& self, const sol::object obj) {
                widget_setbounds (self, obj);
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

        "visible",              sol::property (&Widget::isVisible, &Widget::setVisible),

        "opaque",               sol::property (&Widget::isOpaque, &Widget::setOpaque),

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

        "removefromdesktop",    &Widget::removeFromDesktop,
        "isondesktop",          &Widget::isOnDesktop,
        
        "getbounds",            &Widget::getBounds,
        "setbounds", sol::overload (
            [](Widget& self, double x, double y, double w, double h) {
                self.setBounds (x, y, w, h);
            },
            [](Widget& self, const sol::object& obj) {
                widget_setbounds (self, obj);
            }
        ),
        
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

    /// Attributes.
    // @section attributes
    T_mt["__props"] = lua.create_table().add (
        /// Widget name (string).
        // @field Widget.name
        "name",

        /// Local bounding box.
        // Same as bounds with zero x and y coords
        // @field Widget.localbounds
        "localbounds", 
        
        /// Widget bounding box (kv.Bounds)
        // The coords returned is relative to the top/left of the widget's parent.
        // @field Widget.bounds
        // @usage
        // -- Can also set a table. e.g.
        // widget.bounds = {
        //     x      = 0, 
        //     y      = 0,
        //     width  = 100,
        //     height = 200
        // }
        "bounds",

        /// X position (int).
        // @field Widget.x
        "x",
        
        /// Y position (int).
        // @field Widget.y
        "y",
        
        /// Widget width (int).
        // @field Widget.width
        "width", 
        
        /// Widget height (int).
        // @field Widget.height
        "height", 
        
        /// Widget height (int).
        // @field Widget.height
        "right", 
        
        /// Widget bottom edge (int).
        // @field Widget.bottom
        "bottom",
        
        /// Widget Screen X position (int).
        // @field Widget.screenx
        "screenx", 
        
        /// Widget Screen Y position (int).
        // @field Widget.screeny
        "screeny",

        /// Widget visibility (bool).
        // @field Widget.visible
        "visible"
    );
    
    /// Methods.
    // @section methods
    T_mt["__methods"] = lua.create_table().add (
        /// Repaint.
        // Repaints the entire widget
        // @function Widget:repaint

        /// Repaint section.
        // @function Widget:repaint
        // @int x
        // @int y
        // @int w
        // @int h

        /// Repaint section.
        // @function Widget:repaint
        // @tparam kv.Bounds b Area to repaint
        "repaint",

        /// Resize the widget.
        // @int w New width
        // @int h New height
        // @function Widget:resize
        "resize",

        /// Bring to front.
        // @function Widget:tofront
        // @bool focus If true, will also try to focus this widget.
        "tofront",

        /// To Back.
        // @function Widget:toback
        "toback",

        /// Get bounding box.
        // @function Widget:getbounds
        "getbounds",

        /// Change the bounds.
        // @function Widget:setbounds
        // @int x X pos
        // @int y Y pos
        // @int w Width
        // @int h Height
        "setbounds",

        /// True if the widget is showing on the desktop.
        // @function Widget:isondesktop
        "isondesktop", 

        /// Makes this widget appear as a window on the desktop.
        //
        // Note that before calling this, you should make sure that the widget's opacity is
        // set correctly using setOpaque(). If the widget is non-opaque, the windowing
        // system will try to create a special transparent window for it, which will generally take
        // a lot more CPU to operate (and might not even be possible on some platforms).
        //
        // If the widget is inside a parent widget at the time this method is called, it
        // will first be removed from that parent. Likewise if a widget is on the desktop
        // and is subsequently added to another widget, it'll be removed from the desktop.
        //
        // @function Widget:addtodesktop
        // @int flags Window flags
        // @param[opt] window Native window handle to attach to
        "addtodesktop",

        /// Remove.
        // @function Widget:removefromdesktop
        "removefromdesktop"
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
