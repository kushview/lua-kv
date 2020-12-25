/// A rectangle
// @classmod kv.Rectanglessss

#pragma once

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

namespace kv {
namespace lua {

template<typename T, typename ...Args>
inline static sol::table
new_juce_rectangle (lua_State* L, const char* name, Args&& ...args) {
    using R = juce::Rectangle<T>;
    sol::state_view lua (L);
    sol::table M = lua.create_table();

    M.new_usertype<R> (name, sol::no_constructor,
        sol::call_constructor, sol::factories (
            []() { return R(); },
            [](T x, T y, T w, T h) { return R (x, y, w, h); },
            [](T w, T h) { return R (w, h); },
            [](juce::Point<T> p1, juce::Point<T> p2) { return R (p1, p2); }
        ),

        /// @function Rectangle.from_coords
        "from_coords",      R::leftTopRightBottom,

        /// @field Rectangle.x
        "x",                sol::property (&R::getX, &R::setX),
        "y",                sol::property (&R::getY, &R::setY),

        "center",           sol::readonly_property (&R::getCentre),
        "center_x",         sol::readonly_property (&R::getCentreX),
        "center_y",         sol::readonly_property (&R::getCentreY),

        "width",            sol::property (&R::getWidth, &R::setWidth),
        "height",           sol::property (&R::getHeight, &R::setHeight),

        "left",             sol::property (&R::getX, &R::setLeft),
        "right",            sol::property (&R::getRight, &R::setRight),
        "top",              sol::property (&R::getY, &R::setTop),
        "bottom",           sol::property (&R::getBottom, &R::setBottom),

        "is_empty",         &R::isEmpty,
        "is_finite",        &R::isFinite,
        "translate",        &R::translate,
        "translated",       &R::translated,

        "expand",           &R::expand,
        "expanded", sol::overload (
            [](R& self, T dx, T dy) { return self.expanded (dx, dy); },
            [](R& self, T d)        { return self.expanded (d); }
        ),
        
        "reduce",               &R::reduce,
        "reduced", sol::overload (
            [](R& self, T dx, T dy) { return self.reduced (dx, dy); },
            [](R& self, T d)        { return self.reduced (d); }
        ),

        "slice_top",        &R::removeFromTop,
        "slice_left",       &R::removeFromLeft,
        "slice_right",      &R::removeFromRight,
        "slice_bottom",     &R::removeFromBottom,

        "tointeger",        &R::toNearestInt,
        "toedges",          &R::toNearestIntEdges,
        "tonumber",         &R::toDouble,
        std::forward<Args> (args)...
#if 0        
        "getAspectRatio", sol::overload (
            [](R& self) { return self.getAspectRatio(); },
            [](R& self, bool widthOverHeight) { return self.getAspectRatio (widthOverHeight); }
        ),

        "get_position",     &R::getPosition,
        "set_position", sol::overload (
            sol::resolve<void(Point<T>)> (&R::setPosition),
            sol::resolve<void(T, T)> (&R::setPosition)
        ),

        "getTopLeft",           &R::getTopLeft,
        "getTopRight",          &R::getTopRight,
        "getBottomLeft",        &R::getBottomLeft,
        "getBottomRight",       &R::getBottomRight,
        "getHorizontalRange",   &R::getHorizontalRange,
        "getVerticalRange",     &R::getVerticalRange,
        "setSize",              &R::setSize,
        "setBounds",            &R::setBounds,
        "setX",                 &R::setX,
        "setY",                 &R::setY,
        "setWidth",             &R::setWidth,
        "setHeight",            &R::setHeight,
        "setCentre", sol::overload (
            sol::resolve<void(T, T)> (&R::setCentre),
            sol::resolve<void(Point<T>)> (&R::setCentre)
        ),
        "setHorizontalRange",   &R::setHorizontalRange,
        "setVerticalRange",     &R::setVerticalRange,
        "withX",                &R::withX,
        "withY",                &R::withY,
        "withRightX",           &R::withRightX,
        "withBottomY",          &R::withBottomY,
        "withPosition", sol::overload (
            [](R& self, T x, T y)       { return self.withPosition (x, y); },
            [](R& self, Point<T> pt)    { return self.withPosition (pt); }
        ),
        "withZeroOrigin",       &R::withZeroOrigin,
        "withCentre",           &R::withCentre,

        "withWidth",            &R::withWidth,
        "withHeight",           &R::withHeight,
        "withSize",             &R::withSize,
        "withSizeKeepingCentre",&R::withSizeKeepingCentre,
        "setLeft",              &R::setLeft,
        "withLeft",             &R::withLeft,

        "setTop",               &R::setTop,
        "withTop",              &R::withTop,
        "setRight",             &R::setRight,
        "withRight",            &R::withRight,
        "setBottom",            &R::setBottom,
        "withBottom",           &R::withBottom,
        "withTrimmedLeft",      &R::withTrimmedLeft,
        "withTrimmedRight",     &R::withTrimmedRight,
        "withTrimmedTop",       &R::withTrimmedTop,
        "withTrimmedBottom",    &R::withTrimmedBottom,
        
        "getConstrainedPoint",  &R::getConstrainedPoint,

        "getRelativePoint",     [](R& self, T rx, T ry) { return self.getRelativePoint (rx, ry); },
        "proportionOfWidth",    [](R& self, T p)        { return self.proportionOfWidth (p); },
        "proportionOfHeight",   [](R& self, T p)        { return self.proportionOfHeight (p); },
        "getProportion",        [](R& self, R pr)       { return self.getProportion (pr); }
#endif
    );

    return kv::lua::remove_and_clear (M, name);
}

}}
