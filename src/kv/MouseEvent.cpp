/// A mouse event
// @classmod kv.MouseEvent
// @pragma nostrip

#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_MOUSE_EVENT "MouseEvent"

using namespace juce;

LUAMOD_API
int luaopen_kv_MouseEvent (lua_State* L)
{
    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<MouseEvent> (LKV_TYPE_NAME_MOUSE_EVENT, sol::no_constructor,
        "position", sol::readonly_property ([](MouseEvent& self) {
            return self.position.toFloat();
        }),
        "x",            &MouseEvent::x,
        "y",            &MouseEvent::y,
        "pressure",     &MouseEvent::pressure,
        "orientation",  &MouseEvent::orientation,
        "rotation",     &MouseEvent::rotation,
        "tiltx",        &MouseEvent::tiltX,
        "tilty",        &MouseEvent::tiltY
    );

    sol::stack::push (L, kv::lua::remove_and_clear (M, LKV_TYPE_NAME_MOUSE_EVENT));
    return 1;
}
