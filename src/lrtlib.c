/*
Copyright 2019 Michael Fisher <mfisher@kushview.net>

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

/// Lua RT
// @module lrt

#include <assert.h>
#include "luainc.h"
#include "lrt/lrt.h"

extern int luaopen_dsp_audio (lua_State*);
extern int luaopen_dsp_midi (lua_State*);

static const luaL_Reg lrtmods[] = {
    { "audio", luaopen_dsp_audio },
    { "midi",  luaopen_dsp_midi },
    { NULL, NULL }
};

void lrt_openlibs (lua_State* L, int glb) {
    const int top = lua_gettop (L);
    const luaL_Reg* mod = NULL;
    for (mod = lrtmods; mod->func; mod++) {
        luaL_requiref (L, mod->name, mod->func, glb);
        lua_pop (L, 1);  /* remove lib */
    }
    assert (top == lua_gettop (L));
}
