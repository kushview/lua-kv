/*
Copyright 2019-2020 Michael Fisher <mfisher@kushview.net>

Permission to use, copy, modify, and/or distribute this software for any 
purpose with or without fee is hereby granted, provided that the above 
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
PERFORMANCE OF THIS SOFTWARE.
*/

/// @module kv

#include <math.h>
#include "luainc.h"
#include "lua-kv.h"

#if 1
#define LKV_MINUS_INFINITY_DB   -100.0
#define LKV_UNITY_GAIN          1.0

/// Convert gain to decibels
// @function gaintodb
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
