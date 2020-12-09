
/// A pair of x,y coordinates.
// @classmod kv.Point
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_POINT "Point"

using namespace juce;

LUAMOD_API
int luaopen_kv_Point (lua_State* L)
{
    sol::state_view lua (L);
    using PTF = Point<lua_Number>;
    auto M = lua.create_table();
    M.new_usertype<PTF> (LKV_TYPE_NAME_POINT, sol::no_constructor,
        sol::call_constructor, sol::factories (
            []() { return PTF(); },
            [](lua_Number x, lua_Number y) { return PTF (x, y); }
        ),
        sol::meta_method::to_string, [](PTF& self) {
            return self.toString().toStdString();
        },

        /// True if is the origin point
        // @function Point:is_origin
        // @within Methods
        "is_origin",    &PTF::isOrigin,

        /// True if is finite
        // @function Point:is_finite
        // @within Methods
        "is_finite",    &PTF::isFinite,

        /// X coord
        // @class field
        // @name Point.x
        // @within Attributes
        "x",            sol::property (&PTF::getX, &PTF::setX),

        /// Y coord
        // @class field
        // @name Point.x
        // @within Attributes
        "y",            sol::property (&PTF::getY, &PTF::setY),

        /// Returns a point with the given x coordinate.
        // @param x New x coord
        // @function Point:with_x
        // @treturn kv.Point New point object
        // @within Methods
        "with_x",       &PTF::withX,

        /// Returns a point with the given y coordinate.
        // @param y New y coord
        // @function Point:with_y
        // @treturn kv.Point New point object
        // @within Methods
        "with_y",       &PTF::withY,

        /// Set x and y at the same time.
        // @number x New x coordinate
        // @number y New y coordinate
        // @function Point:set_xy
        // @within Methods
        "set_xy",       &PTF::setXY,

        /// Adds a pair of coordinates to this value.
        // @number x X to add
        // @number y Y to add
        // @function Point:add_xy
        // @within Methods
        "add_xy",       &PTF::addXY,

        /// Move the point by delta x and y
        // @function Point:translated
        // @number deltax
        // @number deltay
        // @within Methods
        "translated",   &PTF::translated,

        /// True if is finite
        // @function distance
        "distance", sol::overload (
            [](PTF& self) { return self.getDistanceFromOrigin(); },
            [](PTF& self, PTF& o) { return self.getDistanceFrom (o); }
        ),

        /// True if is finite
        // @function distance_squared
        "distance_squared", sol::overload (
            [](PTF& self) { return self.getDistanceSquaredFromOrigin(); },
            [](PTF& self, PTF& o) { return self.getDistanceSquaredFrom (o); }
        ),

        /// True if is finite
        // @function angle_to
        "angle_to",     &PTF::getAngleToPoint,

        /// True if is finite
        // @function rotated
        "rotated",      &PTF::rotatedAboutOrigin,
        
        /// Returns the dot product.
        // @function dot_product
        "dot_product",  &PTF::getDotProduct,
        
        /// Convert to integer values.
        // @function to_int
        "to_int",       &PTF::toInt
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, LKV_TYPE_NAME_POINT));
    return 1;
}
