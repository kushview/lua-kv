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

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "luainc.h"
#include "util.h"
#include "lrt/lrt.h"

#define MSG_DATA_SIZE sizeof(uint8_t*)

typedef union _midi_data_t {
    uint8_t*    data;
    uint8_t     bytes [MSG_DATA_SIZE];
} midi_data_t;

struct lrt_midi_message_impl_t {
    midi_data_t data;
    lua_Integer size;
    lua_Number  time;
    bool        allocated;
};

typedef struct lrt_midi_message_impl_t  MidiMessage;

struct lrt_midi_buffer_impl_t {
    uint8_t*    data;
    lua_Integer size;
    lua_Integer used;
};

typedef struct lrt_midi_buffer_impl_t   MidiBuffer;

struct lrt_midi_pipe_impl_t {
    lua_Integer         size;
    lua_Integer         used;
    lrt_midi_buffer_t** buffers;
    int*                mbrefs;
};

typedef struct lrt_midi_pipe_impl_t     MidiPipe;

static void lrt_midi_pipe_alloc (lua_State* L, lrt_midi_pipe_t* pipe) {
    pipe->buffers = malloc (sizeof(lrt_midi_buffer_t*) * (pipe->size + 1));
    pipe->mbrefs  = malloc (sizeof(int) * (pipe->size + 1));

    for (int i = 0; i < pipe->size; ++i) {
        lrt_midi_buffer_new (L, 0);
        pipe->buffers[i] = lua_touserdata (L, -1);
        pipe->mbrefs[i]  = luaL_ref (L, LUA_REGISTRYINDEX);
    }

    pipe->buffers[pipe->size] = NULL;
    pipe->mbrefs[pipe->size]  = LUA_NOREF;
}

lrt_midi_pipe_t* lrt_midi_pipe_new (lua_State* L, int nbuffers) {
    lrt_midi_pipe_t* pipe = lua_newuserdata (L, sizeof (MidiPipe));
    pipe->size = pipe->used = MAX (0, nbuffers);
    lrt_midi_pipe_alloc (L, pipe);
    luaL_setmetatable (L, LRT_MT_MIDI_PIPE);
    return pipe;
}

static void lrt_midi_pipe_free_buffers (lua_State* L, lrt_midi_pipe_t* pipe) {
    for (int i = 0; i < pipe->size; ++i) {
        luaL_unref (L, LUA_REGISTRYINDEX, pipe->mbrefs[i]);
        pipe->mbrefs[i]  = LUA_NOREF;
        pipe->buffers[i] = NULL;
    }

    pipe->size = pipe->used = 0;
    
    if (pipe->buffers != NULL) {
        free (pipe->buffers);
        pipe->buffers = NULL;
    }

    if (pipe->mbrefs != NULL) {
        free (pipe->mbrefs);
        pipe->mbrefs = NULL;
    }
}

void lrt_midi_pipe_clear (lrt_midi_pipe_t* pipe, int index) {
    if (index < 0) {
        for (int i = 0; i < pipe->used; ++i) {
            pipe->buffers[i]->used = 0;
        }
    } else {
        pipe->buffers[index]->used = 0;
    }
}

void lrt_midi_pipe_resize (lua_State* L, lrt_midi_pipe_t* pipe, int nbufs) {
    if (nbufs == pipe->used) {
        return;
    }

    if (nbufs < pipe->size) {
        pipe->used = nbufs;
        return;
    }

    lrt_midi_pipe_free_buffers (L, pipe);
    pipe->used = pipe->size = nbufs;
    lrt_midi_pipe_alloc (L, pipe);
}

lrt_midi_buffer_t* lrt_midi_pipe_get (lrt_midi_pipe_t* pipe, int index) {
    return pipe->buffers [index];
}

//=============================================================================
static uint8_t* lrt_midi_message_data (lrt_midi_message_t* msg) {
    return msg->allocated ? msg->data.data : (uint8_t*) msg->data.bytes;
}

static void lrt_midi_message_update (lrt_midi_message_t* msg, 
                                     uint8_t*            data, 
                                     lua_Integer         size)
{
    memcpy (lrt_midi_message_data (msg), data, size);
    msg->size = size;
}

static void lrt_midi_message_init (lrt_midi_message_t* msg) {
    msg->allocated = false;
    for (size_t i = 0; i < sizeof (uint8_t*); ++i)
        msg->data.bytes[i] = 0x00;
    msg->time = 0.0;
    msg->data.bytes[0] = 0xf0;
    msg->data.bytes[1] = 0xf7;
    msg->size = 2;
}

//=============================================================================
static lrt_midi_buffer_t* lrt_midi_buffer_init (lrt_midi_buffer_t* buf, size_t size) {
    buf->used = 0;

    if (size > 0) {
        buf->size = size;
        buf->data = malloc (buf->size);
    } else {
        buf->size = 0;
        buf->data = NULL;
    }
   
    return buf;
}

lrt_midi_buffer_t* lrt_midi_buffer_new (lua_State* L, size_t size) {
    size = MAX(0, size);
    MidiBuffer* buf = lua_newuserdata (L, sizeof (MidiBuffer));
    lrt_midi_buffer_init (buf, size);
    luaL_setmetatable (L, LRT_MT_MIDI_BUFFER);
    return buf;
}

static void lrt_midi_buffer_free_data (lrt_midi_buffer_t* buf) {
    if (buf->data != NULL) {
        free (buf->data);
        buf->data = NULL;
    }
    buf->size = buf->used = 0;
}

static void lrt_midi_buffer_free (lrt_midi_buffer_t* buf) {
    lrt_midi_buffer_free_data (buf);
    free (buf);
}

void lrt_midi_buffer_clear (lrt_midi_buffer_t* buf) {
    buf->used = 0;
}

static void lrt_midi_buffer_swap (lrt_midi_buffer_t* a, lrt_midi_buffer_t* b) {
    a->size = a->size + b->size;
    b->size = a->size - b->size;
    a->size = a->size - b->size;

    a->used = a->used + b->used;
    b->used = a->used - b->used;
    a->used = a->used - b->used;

    a->data = (uint8_t*)((uintptr_t)a->data + (uintptr_t)b->data);
    b->data = (uint8_t*)((uintptr_t)a->data - (uintptr_t)b->data);
    a->data = (uint8_t*)((uintptr_t)a->data - (uintptr_t)b->data);
}

void lrt_midi_buffer_insert (lrt_midi_buffer_t* buf, 
                             const uint8_t*     bytes, 
                             size_t             len, 
                             int                frame)
{
    const int32_t iframe = (int32_t) frame;
    const int16_t ulen   = (uint16_t) len;
    const size_t needed = len + sizeof (int32_t) + sizeof (uint16_t);
    if (buf->size < buf->used + needed) {
        buf->data = realloc (buf->data, buf->size + needed);
        buf->size += needed;
    }

    uint8_t* iter = buf->data;
    const uint8_t* const end  = buf->data + buf->used;
    while (iter < end) {
        int32_t evfr = *(int32_t*) iter;
        
        if (evfr >= iframe) {
            memmove (iter + needed, iter, end - iter);
            break;
        }

        iter += lrt_midi_buffer_iter_total_size (iter);
    }

    memcpy (iter, &iframe, sizeof (int32_t));
    iter += sizeof (int32_t);
    memcpy (iter, &ulen, sizeof (uint16_t));
    iter += sizeof (uint16_t);
    memcpy (iter, bytes, len);
    buf->used += needed;
}

lrt_midi_buffer_iter_t lrt_midi_buffer_begin (lrt_midi_buffer_t* buf) {
    return buf->data;
}

lrt_midi_buffer_iter_t lrt_midi_buffer_end (lrt_midi_buffer_t* buf) {
    return buf->data + buf->used;
}

lrt_midi_buffer_iter_t lrt_midi_buffer_next (lrt_midi_buffer_t*     buf, 
                                             lrt_midi_buffer_iter_t iter)
{
    iter += lrt_midi_buffer_iter_total_size (iter);
    return ((uint8_t*)iter) <= (buf->data + buf->used)
        ? iter : (buf->data + buf->used);
}

//=============================================================================
static int midimessage_new (lua_State* L) {
    const int nargs = lua_gettop (L);

    MidiMessage* msg = lua_newuserdata (L, sizeof (MidiMessage));
    luaL_setmetatable (L, LRT_MT_MIDI_MESSAGE);
    lrt_midi_message_init (msg);

    if (nargs == 1 && lua_isinteger (L, 1)) {
        uint8_t* data = lrt_midi_message_data (msg);
        lua_Integer src = lua_tointeger (L, 1);
        memcpy (data, (uint8_t*)&src, 3);
    }

    return 1;
}

static int midimessage_gc (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    if (msg->allocated) {
        free (msg->data.data);
    }
    return 0;
}

static int midimessage_update (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    
    if (lua_gettop (L) >= 3) {
        uint8_t* data    = lua_touserdata (L, 2);
        lua_Integer size = lua_tointeger (L, 3);
        if (size > 0) {
            lrt_midi_message_update (msg, data, size);
        }
    }

    return 0;
}

static int midimessage_channel (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    uint8_t* data = lrt_midi_message_data (msg);
    
    if ((data[0] & 0xf0) == (uint8_t) 0xf0) {
        lua_pushinteger (L, 0);
        return 1;
    }

    if (lua_gettop (L) >= 2) {
        lua_Integer channel = lua_tointeger (L, 2);
        if (channel >= 1 && channel <= 16)
            data[0] = (data[0] & (uint8_t)0xf0) | (uint8_t)(channel - 1);
    }

    lua_pushinteger (L, (data[0] & 0xf) + 1);
    return 1;
}

static int midimessage_isnoteon (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    uint8_t* data = lrt_midi_message_data (msg);
    lua_pushboolean (L, (data[0] & 0xf0) == 0x90 && data[2] != 0);
    return 1;
}

static int midimessage_isnoteoff (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    uint8_t* data = lrt_midi_message_data (msg);
    lua_pushboolean (L, (data[0] & 0xf0) == 0x80);
    return 1;
}

static int midimessage_controller (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    uint8_t* data = lrt_midi_message_data (msg);
    lua_pushboolean (L, (data[0] & 0xf0) == 0xb0);
    return 1;
}

static int midimessage_tostring (lua_State* L) {
    MidiMessage* msg = lua_touserdata (L, 1);
    uint8_t* data = lrt_midi_message_data (msg);
    char buffer [128];
    snprintf (buffer, 128, "data1=0x%02x data2=0x%02x data3=0x%02x",
                            data[0], data[1], data[2]);
    lua_pushstring (L, buffer);
    return 1;
}

static const luaL_Reg midimessage_m[] = {
    { "__gc",       midimessage_gc },
    { "__tostring", midimessage_tostring },
    { "update",     midimessage_update },
    { "channel",    midimessage_channel },
    { "controller", midimessage_controller },
    { "noteon",     midimessage_isnoteon },
    { "noteoff",    midimessage_isnoteoff },
    { NULL, NULL }
};

//=============================================================================
static int midibuffer_new (lua_State* L) {
    size_t size = (size_t)(lua_gettop(L) > 0 ? MAX(0, lua_tointeger (L, 1)) : 0);
    lrt_midi_buffer_new (L, size);
    return 1;
}

static int midibuffer_gc (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    lrt_midi_buffer_free_data (buf);
    return 0;
}

static int midibuffer_insert (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    if (lua_gettop (L) == 3) {
        lua_Integer data = lua_tointeger (L, 3);
        lrt_midi_buffer_insert (buf, (uint8_t*)&data, 3, lua_tointeger (L, 2));
        lua_pushinteger (L, 3);
    } else {
        int size = lua_gettop (L) - 2;

        if (size > 1) {
            uint8_t data [size];
            int frame = lua_tointeger (L, 2);

            for (int i = 0; i < size; ++i) {
                data[i] = (uint8_t) lua_tointeger (L, i + 3);
            }

            lrt_midi_buffer_insert (buf, data, (size_t)size, frame);
        }

        lua_pushinteger (L, (lua_Integer) size);
    }
    return 1;
}

static int midibuffer_clear (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    buf->used = 0;
    return 0;
}

static int midibuffer_capacity (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    lua_pushinteger (L, (lua_Integer) buf->size);
    return 1;
}

static int midibuffer_reserve (lua_State* L) {
    MidiBuffer* buf = luaL_checkudata (L, 1, LRT_MT_MIDI_BUFFER);
    if (buf == NULL || lua_gettop (L) < 2) {
        lua_pushboolean (L, false);
    } else {
        lua_Integer new_size = MAX(0, lua_tointeger (L, 2));
        if (new_size > buf->size) {
            buf->data = realloc (buf->data, new_size);
            buf->size = new_size;
        }
    }
    return 1;
}

static int midibuffer_swap (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    
    switch (lua_type (L, 2)) {
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA: {
            lrt_midi_buffer_swap (buf, lua_touserdata (L, 2));
            break;
        }

        default: {
            return luaL_error (L, "unsupported type: %s", luaL_typename (L, 2));
            break;
        }
    }

    return 0;
}

static int midibuffer_events_f (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lrt_midi_buffer_iter_t iter = lua_touserdata (L, lua_upvalueindex (2));
    
    if (iter == lrt_midi_buffer_end (buf)) {
        lua_pushnil (L);
        return 1;
    }

    lua_pushlightuserdata (L, lrt_midi_buffer_next (buf, iter));
    lua_copy (L, -1, lua_upvalueindex (2));
    lua_pop (L, -1);

    lua_pushlightuserdata (L, lrt_midi_buffer_iter_data (iter));
    lua_pushinteger (L, lrt_midi_buffer_iter_size (iter));
    lua_pushinteger (L, lrt_midi_buffer_iter_frame (iter));
    
    return 3;
}

static int midibuffer_events_msg_f (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lrt_midi_buffer_iter_t iter = lua_touserdata (L, lua_upvalueindex (2));
    MidiMessage* msg = lua_touserdata (L, lua_upvalueindex (3));

    if (iter == lrt_midi_buffer_end (buf)) {
        lua_pushnil (L);
        return 1;
    }

    lua_pushlightuserdata (L, lrt_midi_buffer_next (buf, iter));
    lua_copy (L, -1, lua_upvalueindex (2));
    lua_pop (L, -1);

    lrt_midi_message_update (msg, lrt_midi_buffer_iter_data (iter),
                                  lrt_midi_buffer_iter_size (iter));
    lua_pushinteger (L, lrt_midi_buffer_iter_frame (iter));
    return 1;
}

static int midibuffer_events (lua_State* L) {
    MidiBuffer* buf = lua_touserdata (L, 1);
    int nargs = lua_gettop (L) - 1;
    lua_pushlightuserdata (L, buf);
    lua_pushlightuserdata (L, lrt_midi_buffer_begin (buf));
    if (nargs > 0) {
        lua_pushvalue (L, 2);
        lua_pushcclosure (L, &midibuffer_events_msg_f, 3);
    } else {
        lua_pushcclosure (L, &midibuffer_events_f, 2);
    }

    return 1;
}

static const luaL_Reg midibuffer_m[] = {
    { "__gc",       midibuffer_gc },
    { "capacity",   midibuffer_capacity },
    { "clear",      midibuffer_clear },
    { "events",     midibuffer_events },
    { "insert",     midibuffer_insert },
    { "reserve",    midibuffer_reserve },
    { "swap",       midibuffer_swap },
    { NULL, NULL }
};

//=============================================================================
static int midipipe_new (lua_State* L) {
    lua_Integer nbuffers = 0;
    if (lua_gettop (L) > 0) {
        nbuffers = MAX (0, lua_tointeger (L, 1));
    }

    lrt_midi_pipe_new (L, nbuffers);
    return 1;
}

static int midipipe_gc (lua_State* L) {
    MidiPipe* pipe = lua_touserdata (L, 1);
    lrt_midi_pipe_free_buffers (L, pipe);
    return 0;
}

static int midipipe_len (lua_State* L) {
    MidiPipe* pipe = lua_touserdata (L, 1);
    lua_pushinteger (L, pipe->size);
    return 1;
}

static int midipipe_tostring (lua_State* L) {
    MidiPipe* pipe = luaL_checkudata (L, 1, LRT_MT_MIDI_PIPE);
    lua_pushfstring (L, "MidiPipe: nbuffers=%d", pipe->size);
    return 1;
}

static int midipipe_clear (lua_State* L) {
    MidiPipe* pipe = lua_touserdata (L, 1);

    if (lua_gettop(L) > 1) {
        lua_Integer i = lua_tointeger (L, 2) - 1;
        lrt_midi_buffer_clear (pipe->buffers [i]);
    } else {
        for (int i = 0; i < pipe->size; ++i) {
            lrt_midi_buffer_clear (pipe->buffers [i]);
        }
    }

    return 0;
}

static int midipipe_get (lua_State* L) {
    MidiPipe* pipe = lua_touserdata (L, 1);
    lua_Integer index = lua_tointeger (L, 2) - 1;
    if (index >= 0 && index < pipe->size) {
        lua_rawgeti (L, LUA_REGISTRYINDEX, pipe->mbrefs [index]);
    } else {
        lua_pushnil (L);
    }
    return 1;
}

static int midipipe_index (lua_State* L) {
    MidiPipe* pipe = lua_touserdata (L, 1);
    if (lua_isinteger (L, 2)) {
        lua_Integer index = lua_tointeger (L, 3) - 1;
        if (index >= 0 && index < pipe->size) {
            lua_rawgeti (L, LUA_REGISTRYINDEX, pipe->mbrefs [index]);
        } else {
            lua_pushnil (L);
        }
    } else {
        luaL_getmetafield (L, 1, lua_tostring (L, 2));
        lua_pushvalue (L, -1);
    }

    return 1;
}

static int midipipe_newindex (lua_State* L) {
    return 0;
}

static const luaL_Reg midipipe_m[] = {
    { "__gc",       midipipe_gc },
    { "__len",      midipipe_len },
    { "__tostring", midipipe_tostring },
    { "clear",      midipipe_clear },
    { "get",        midipipe_get },
    { NULL, NULL }
};

//=============================================================================
static int f_msg3bytes (lua_State* L, uint8_t status) {
    lua_Integer block = 0;
    uint8_t* data = (uint8_t*) &block;
    data[0] = status | (uint8_t)(lua_tointeger (L, 1) - 1);
    data[1] = (uint8_t) lua_tointeger (L, 2);
    data[2] = (uint8_t) lua_tointeger (L, 3);
    lua_pushinteger (L, (lua_Integer) block);
    return 1;
}

static int f_controller (lua_State* L)  { return f_msg3bytes (L, 0xb0); }
static int f_noteon (lua_State* L)      { return f_msg3bytes (L, 0x90); }
static int f_noteoff (lua_State* L)     { return f_msg3bytes (L, 0x80); }

static const luaL_Reg midi_f[] = {
    { "Message",    midimessage_new },
    { "Buffer",     midibuffer_new },
    { "Pipe",       midipipe_new },
    { "controller", f_controller },
    { "noteon",     f_noteon },
    { "noteoff",    f_noteoff },
    { NULL, NULL }
};

LRT_EXPORT int luaopen_midi (lua_State* L) {
    luaL_newmetatable (L, LRT_MT_MIDI_BUFFER);
    lua_pushvalue (L, -1);               /* duplicate the metatable */
    lua_setfield (L, -2, "__index");     /* mt.__index = mt */
    luaL_setfuncs (L, midibuffer_m, 0);

    luaL_newmetatable (L, LRT_MT_MIDI_MESSAGE);
    lua_pushvalue (L, -1);               /* duplicate the metatable */
    lua_setfield (L, -2, "__index");     /* mt.__index = mt */
    luaL_setfuncs (L, midimessage_m, 0);

    luaL_newmetatable (L, LRT_MT_MIDI_PIPE);
    lua_pushvalue (L, -1);               /* duplicate the metatable */
    lua_setfield (L, -2, "__index");     /* mt.__index = mt */
    luaL_setfuncs (L, midipipe_m, 0);

    luaL_newlib (L, midi_f);
    return 1;
}
