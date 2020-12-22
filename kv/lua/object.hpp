
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

}}
