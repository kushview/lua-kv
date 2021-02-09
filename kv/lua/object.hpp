
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
T* object_userdata (const sol::table& proxy)
{
    if (! proxy.valid())
        return nullptr;
    auto mt = proxy [sol::metatable_key];
    T* result = nullptr;
    
    try {
        if (mt["__impl"].get_type() == sol::type::userdata)
        {
            sol::userdata ud = mt["__impl"];
            result = (T*)ud.pointer();
        }
    } catch (const std::exception&) {}
    
    return result;
}

}}
