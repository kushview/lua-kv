#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "luainc.h"
#include "util.h"
#include "lua-kv.h"

#define MSG_DATA_SIZE sizeof (uint8_t*)

typedef union _midi_data_t {
    uint8_t*    data;
    uint8_t     bytes [MSG_DATA_SIZE];
} midi_data_t;

struct kv_midi_message_impl_t {
    midi_data_t data;
    lua_Integer size;
    lua_Number  time;
    bool        allocated;
};

typedef struct kv_midi_message_impl_t  MidiMessage;

struct kv_midi_buffer_impl_t {
    uint8_t*    data;
    lua_Integer size;
    lua_Integer used;
};

typedef struct kv_midi_buffer_impl_t   MidiBuffer;

struct kv_midi_pipe_impl_t {
    lua_Integer         size;
    lua_Integer         used;
    kv_midi_buffer_t** buffers;
    int*                mbrefs;
};

typedef struct kv_midi_pipe_impl_t     MidiPipe;

static void kv_midi_pipe_alloc (lua_State* L, kv_midi_pipe_t* pipe) {
    pipe->buffers = malloc (sizeof(kv_midi_buffer_t*) * (pipe->size + 1));
    pipe->mbrefs  = malloc (sizeof(int) * (pipe->size + 1));

    for (int i = 0; i < pipe->size; ++i) {
        kv_midi_buffer_new (L, 0);
        pipe->buffers[i] = lua_touserdata (L, -1);
        pipe->mbrefs[i]  = luaL_ref (L, LUA_REGISTRYINDEX);
    }

    pipe->buffers[pipe->size] = NULL;
    pipe->mbrefs[pipe->size]  = LUA_NOREF;
}

kv_midi_pipe_t* kv_midi_pipe_new (lua_State* L, int nbuffers) {
    kv_midi_pipe_t* pipe = lua_newuserdata (L, sizeof (MidiPipe));
    pipe->size = pipe->used = MAX (0, nbuffers);
    kv_midi_pipe_alloc (L, pipe);
    luaL_setmetatable (L, LKV_MT_MIDI_PIPE);
    return pipe;
}

static void kv_midi_pipe_free_buffers (lua_State* L, kv_midi_pipe_t* pipe) {
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

void kv_midi_pipe_free (lua_State* L, kv_midi_pipe_t* pipe) {
    kv_midi_pipe_free_buffers (L, pipe);
    free (pipe);
}

int kv_midi_pipe_size (kv_midi_pipe_t* pipe) {
    return (int) pipe->size;
}

void kv_midi_pipe_clear (kv_midi_pipe_t* pipe, int index) {
    if (index < 0) {
        for (int i = 0; i < pipe->used; ++i) {
            pipe->buffers[i]->used = 0;
        }
    } else {
        pipe->buffers[index]->used = 0;
    }
}

void kv_midi_pipe_resize (lua_State* L, kv_midi_pipe_t* pipe, int nbufs) {
    if (nbufs == pipe->used) {
        return;
    }

    if (nbufs < pipe->size) {
        pipe->used = nbufs;
        return;
    }

    kv_midi_pipe_free_buffers (L, pipe);
    pipe->used = pipe->size = nbufs;
    kv_midi_pipe_alloc (L, pipe);
}

kv_midi_buffer_t* kv_midi_pipe_get (kv_midi_pipe_t* pipe, int index) {
    return pipe->buffers [index];
}

//=============================================================================
uint8_t* kv_midi_message_data (kv_midi_message_t* msg) {
    return msg->allocated ? msg->data.data : (uint8_t*) msg->data.bytes;
}

void kv_midi_message_update (kv_midi_message_t* msg, 
                                    uint8_t*           data, 
                                    lua_Integer        size)
{
    memcpy (kv_midi_message_data (msg), data, size);
    msg->size = size;
}

static void kv_midi_message_init (kv_midi_message_t* msg) {
    msg->allocated = false;
    for (size_t i = 0; i < sizeof (uint8_t*); ++i)
        msg->data.bytes[i] = 0x00;
    msg->time = 0.0;
    msg->data.bytes[0] = 0xf0;
    msg->data.bytes[1] = 0xf7;
    msg->size = 2;
}

void kv_midi_message_reset (kv_midi_message_t* msg) {
    if (msg->allocated) {
        free (msg->data.data);
    }
    kv_midi_message_init (msg);
}

kv_midi_message_t* kv_midi_message_new (lua_State* L, uint8_t* data, size_t size, bool push) {
    MidiMessage* msg = (MidiMessage*) malloc (sizeof (MidiMessage));
    kv_midi_message_init (msg);

    if (NULL != data && size > 0 && size < 4) {
        kv_midi_message_update (msg, data, (lua_Integer) size);
    }

    if (push) {
        MidiMessage** userdata = lua_newuserdata (L, sizeof (MidiMessage**));
        luaL_setmetatable (L, LKV_MT_MIDI_MESSAGE);
        *userdata = msg;
    }

    return msg;
}

void kv_midi_message_free (kv_midi_message_t* msg) {
    if (msg->allocated) {
        free (msg->data.data);
    }
    free (msg);
}

//=============================================================================
static kv_midi_buffer_t* kv_midi_buffer_init (kv_midi_buffer_t* buf, size_t size) {
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

kv_midi_buffer_t* kv_midi_buffer_new (lua_State* L, size_t size) {
    size = MAX(0, size);
    MidiBuffer* buf = lua_newuserdata (L, sizeof (MidiBuffer));
    kv_midi_buffer_init (buf, size);
    luaL_setmetatable (L, LKV_MT_MIDI_BUFFER);
    return buf;
}

static void kv_midi_buffer_free_data (kv_midi_buffer_t* buf) {
    if (buf->data != NULL) {
        free (buf->data);
        buf->data = NULL;
    }
    buf->size = buf->used = 0;
}

void kv_midi_buffer_free (kv_midi_buffer_t* buf) {
    kv_midi_buffer_free_data (buf);
    free (buf);
}

void kv_midi_buffer_clear (kv_midi_buffer_t* buf) {
    buf->used = 0;
}

size_t kv_midi_buffer_capacity (kv_midi_buffer_t* buf) {
    return (size_t) buf->size;
}

void kv_midi_buffer_swap (kv_midi_buffer_t* a, kv_midi_buffer_t* b) {
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

void kv_midi_buffer_insert (kv_midi_buffer_t* buf, 
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

        iter += kv_midi_buffer_iter_total_size (iter);
    }

    memcpy (iter, &iframe, sizeof (int32_t));
    iter += sizeof (int32_t);
    memcpy (iter, &ulen, sizeof (uint16_t));
    iter += sizeof (uint16_t);
    memcpy (iter, bytes, len);
    buf->used += needed;
}

kv_midi_buffer_iter_t kv_midi_buffer_begin (kv_midi_buffer_t* buf) {
    return buf->data;
}

kv_midi_buffer_iter_t kv_midi_buffer_end (kv_midi_buffer_t* buf) {
    return buf->data + buf->used;
}

kv_midi_buffer_iter_t kv_midi_buffer_next (kv_midi_buffer_t*     buf, 
                                           kv_midi_buffer_iter_t iter)
{
   #ifdef _MSC_VER
    ((uintptr_t) iter) += kv_midi_buffer_iter_total_size ((uintptr_t) iter);
   #else
    iter += kv_midi_buffer_iter_total_size (iter);
   #endif
    return ((uint8_t*)iter) <= (buf->data + buf->used)
        ? iter : (buf->data + buf->used);
}
