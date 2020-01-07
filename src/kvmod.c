
#include <math.h>
#include "luainc.h"
#include "lua-kv.h"

#if 0
#define LKV_MINUS_INFINITY_DB   -100.0
#define LKV_UNITY_GAIN          1.0

static int f_gaintodb (lua_State* L) {
    int isnum = 0;
    lua_Number gain = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) gain = LKV_UNITY_GAIN;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = LKV_MINUS_INFINITY_DB;
    lua_pushnumber (L, gain > 0.0 ? fmax (infinity, log10 (gain) * 20.0) : infinity);
    return 1;
}

static int f_dbtogain (lua_State* L) {
    int isnum = 0;
    lua_Number db = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) db = 1.0;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = LKV_MINUS_INFINITY_DB;
    lua_pushnumber (L, db > infinity ? pow (10.f, db * 0.05) : 0.0);
    return 1;
}

static const luaL_Reg lrt_f[] = {
    { "gaintodb",   f_gaintodb },
    { "dbtogain",   f_dbtogain },
    { NULL, NULL }
};
#else
static const luaL_Reg lrt_f[] = {
    { NULL, NULL }
};
#endif

LKV_EXPORT int luaopen_kv (lua_State* L) {
    luaL_newlib (L, lrt_f);
    return 1;
}
