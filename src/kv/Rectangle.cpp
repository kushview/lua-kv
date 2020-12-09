/// A rectangle.
// The value type for this is a 32bit float
// @classmod kv.Rectangle
// @pragma nostrip

#include "juce_rectangle.hpp"

#define LKV_TYPE_NAME_RECTANGLE "Rectangle"

using namespace juce;

LUAMOD_API
int luaopen_kv_Rectangle (lua_State* L) {
    using R = Rectangle<float>;

    auto M = kv::lua::new_juce_rectangle<float> (L, LKV_TYPE_NAME_RECTANGLE,
        sol::meta_method::to_string, [](R& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_RECTANGLE);
        }
    );
    
    sol::stack::push (L, M);
    return 1;
}
