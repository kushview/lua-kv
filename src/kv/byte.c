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

/// MIDI utilities.
// @author Michael Fisher
// @module kv.byte

#include <lauxlib.h>
#include <stdint.h>
#include "packed.h"

/// Pack 4 bytes in a 64bit integer.
// Undefined params are treated as zero
// @function pack
// @int b1 First byte
// @int b2 Second byte
// @int b3 Third byte
// @int b4 Fourth byte
// @treturn int Packed integer
static int f_pack (lua_State* L) {
    kv_packed_t msg = { .packed = 0x0 };

    switch (lua_gettop (L)) {
        case 3:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = (uint8_t) lua_tointeger (L, 3);
            msg.data[3] = 0x00;
            break;

        case 0:
            break;

        case 1:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = msg.data[2] = msg.data[3] = 0x00;
            break;

        case 2:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = msg.data[3] = 0x00;
            break;

        case 4: // >= 4
        default:
            msg.data[0] = (uint8_t) lua_tointeger (L, 1);
            msg.data[1] = (uint8_t) lua_tointeger (L, 2);
            msg.data[2] = (uint8_t) lua_tointeger (L, 3);
            msg.data[3] = (uint8_t) lua_tointeger (L, 4);
            break;
    }

    lua_pushinteger (L, msg.packed);
    return 1;
}

static const luaL_Reg midi_f[] = {
    { "pack",   f_pack },
    { NULL, NULL }
};

LUAMOD_API
int luaopen_kv_byte (lua_State* L) {
    luaL_newlib (L, midi_f);
    return 1;
}
