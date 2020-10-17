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

#include <assert.h>
#include "luainc.h"
#include "lua-kv.h"

extern int luaopen_kv (lua_State*);
extern int luaopen_kv_audio (lua_State*);
extern int luaopen_kv_midi (lua_State*);
extern int luaopen_kv_vector (lua_State*);

static const luaL_Reg lrtmods[] = {
    { "kv",        luaopen_kv },
    { "kv.audio",  luaopen_kv_audio },
    { "kv.midi",   luaopen_kv_midi },
    { "kv.vector", luaopen_kv_vector },
    { NULL, NULL }
};

void kv_openlibs (lua_State* L, int glb) {
    const int top = lua_gettop (L);
    const luaL_Reg* mod = NULL;
    for (mod = lrtmods; mod->func; mod++) {
        luaL_requiref (L, mod->name, mod->func, glb);
        lua_pop (L, 1);  /* remove lib */
    }
    assert (top == lua_gettop (L));
}
