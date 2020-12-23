
#pragma once

#include <sol/sol.hpp>

namespace kv {
namespace lua {

template<class DerivedType>
class Object : public DerivedType {
public:
    Object() = default;
    ~Object() = default;
};

template<typename T> static
T* object_userdata (const sol::table& proxy) {
    auto mt = proxy[sol::metatable_key];
    return mt["__impl"].get_or<T*> (nullptr);
}

}}
