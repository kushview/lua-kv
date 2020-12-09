/// A drawing context.
// @classmod kv.Graphics
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

using namespace juce;

LUAMOD_API
int luaopen_kv_Graphics (lua_State* L) {
    sol::state_view lua (L);

    auto M = lua.create_table();
    M.new_usertype<Graphics> ("Graphics", sol::no_constructor,
        /// Change the color
        // @function set_color
        // @int color New ARGB color as integer. e.g.`0xAARRGGBB`
        "set_color", sol::overload (
            [](Graphics& g, int color) { g.setColour (Colour (color)); }
        ),
        
        /// Draw some text
        // @function draw_text
        // @string text Text to draw
        // @int x Horizontal position
        // @int y Vertical position
        // @int width Width of containing area
        // @int height Height of containing area
        "draw_text", sol::overload (
            [](Graphics& g, std::string t, int x, int y, int w, int h) {
                g.drawText (t, x, y, w, h, Justification::centred, true);
            }
        ),

        /// Fill the entire drawing area.
        // @function fill_all
        "fill_all", sol::overload (
            [](Graphics& g)                 { g.fillAll(); },
            [](Graphics& g, int color)      { g.fillAll (Colour (color)); }
        )
    );
    sol::stack::push (L, kv::lua::remove_and_clear (M, "Graphics"));
    return 1;
}
