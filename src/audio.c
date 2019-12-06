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
#include <math.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "luainc.h"
#include "util.h"
#include "lrt/lrt.h"

#define MT_AUDIO_BUFFER       "kv.audio.Buffer"
#define MT_AUDIO_BUFFER_DATA  MT_AUDIO_BUFFER ".Data"

struct lrt_audio_buffer_impl_t {
    int nframes;                // sample count
    int nchannels;              // channel count
    size_t size;                // size of allocated data. zero if referenced
    char* data;                 // allocated data block
    lrt_sample_t** channels;        // channels actually used
    lrt_sample_t* prealloc [32];    // pre-allocated channel space
    bool cleared;               // true if buffer has been cleared
};

typedef struct lrt_audio_buffer_impl_t AudioBuffer;

//=============================================================================
static int audiobuffer_new (lua_State* L);
static void audiobuffer_allocate_data (AudioBuffer* buf);

//=============================================================================
static void audiobuffer_init (AudioBuffer* buf) {
    buf->nframes = buf->nchannels = 0;
    buf->size = 0;
    buf->data = NULL;
    memset (buf->prealloc, 0, sizeof(lrt_sample_t*) * 32);
    buf->channels = (lrt_sample_t**) buf->prealloc;
    buf->cleared = false;
}

static void audiobuffer_free_data (AudioBuffer* buf) {
    if (buf->size != 0 && buf->data != NULL) {
        free (buf->data);
        buf->size = 0;
        buf->data = NULL;  
    }
}

static int audiobuffer_gc (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    audiobuffer_free_data (buf);
    return 0;
}

//=============================================================================
static void audiobuffer_allocate_data (AudioBuffer* buf) {
    size_t list_size = (size_t)(buf->nchannels + 1) * sizeof(lrt_sample_t*);
    size_t required_alignment = (size_t) alignof (lrt_sample_t);
    size_t alignment_overflow = list_size % required_alignment;
    if (alignment_overflow > 0)
        list_size += alignment_overflow - required_alignment;

    buf->size = (size_t)buf->nchannels * (size_t)buf->nframes * sizeof(lrt_sample_t) + list_size + 32;
    buf->data = malloc (buf->size);
    buf->channels = (lrt_sample_t**) buf->data;
    lrt_sample_t* chan = (lrt_sample_t*)(buf->data + list_size);

    int i;
    for (i = 0; i < buf->nchannels; ++i) {
        buf->channels[i] = chan;
        chan += buf->nframes;
    }

    buf->channels[buf->nchannels] = NULL;
    buf->cleared = false;
}

static void audiobuffer_allocate_channels (AudioBuffer* buf, lrt_sample_t* const* data, int offset) {
    assert (offset >= 0);
    
    if (buf->nchannels < 32) {
        buf->channels = (lrt_sample_t**) buf->prealloc;
    } else {
        buf->size = (size_t)(buf->nchannels + 1) * sizeof (lrt_sample_t*);
        buf->data = malloc (buf->size);
        buf->channels = (lrt_sample_t**)(buf->data);
    }

    for (int i = 0; i < buf->nchannels; ++i) {
        assert (data[i] != NULL);
        buf->channels[i] = data[i] + offset;
    }

    buf->channels[buf->nchannels] = NULL;
    buf->cleared = false;
}

void lrt_audiobuffer_referto (void* handle, lrt_sample_t* const* data, int nchannels, int nframes) {
    AudioBuffer* buf = (AudioBuffer*) handle;
    buf->nchannels = nchannels;
    buf->nframes   = nframes;
    if (buf->size != 0)
        audiobuffer_free_data (buf);
    audiobuffer_allocate_channels (buf, data, 0);
}

//=============================================================================
static void audiobuffer_clear_all (AudioBuffer* buf) {
    if (buf->cleared)
        return;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c], 0, sizeof(lrt_sample_t) * (size_t)buf->nframes);
    buf->cleared = true;
}

static void audiobuffer_clear_range (AudioBuffer* buf, int start, int count) {
    if (buf->cleared)
        return;
    if (start == 0 && count == buf->nframes)
        buf->cleared = true;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c] + start, 0, sizeof(lrt_sample_t) * (size_t)buf->nframes);
}

static void audiobuffer_clear_channel_range (AudioBuffer* buf, int channel, int start, int count) {
    if (buf->cleared)
        return;
    memset (buf->channels[channel] + start, 0, sizeof(lrt_sample_t) * count);
}

static void audiobuffer_clear_channel (AudioBuffer* buf, int channel) {
    audiobuffer_clear_channel_range (buf, channel, 0, buf->nframes);
    if (channel == 0 && buf->nchannels == 1)
        buf->cleared = true;
}

//=============================================================================
static int audiobuffer_clear (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    if (buf == NULL)
        return 0;

    switch (lua_gettop (L)) {
        case 0: break;
        case 1: audiobuffer_clear_all (buf); break;
        
        case 2: {
            audiobuffer_clear_channel (buf, lua_tointeger (L, 2));
        } break;
        
        case 3: {
            audiobuffer_clear_range (buf,
                lua_tointeger (L, 2), 
                lua_tointeger (L, 3));
        } break;
        
        default:
        case 4: {
            audiobuffer_clear_channel_range (buf,
                lua_tointeger (L, 2),
                lua_tointeger (L, 3),
                lua_tointeger (L, 4));
        } break;
    }

    return 0;
}

static int audiobuffer_cleared (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushboolean (L, buf != NULL ? buf->cleared : false);
    return 1;
}

//=============================================================================
/** returns the number of samples in the buffer */
static int audiobuffer_length (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushinteger (L, buf != NULL ? buf->nframes : 0);
    return 1;
}

//=============================================================================
/** returns the number of channels held in the buffer */
static int audiobuffer_channels (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushinteger (L, buf != NULL ? buf->nchannels : 0);
    return 1;
}

//=============================================================================
/** returns the number of channels held in the buffer */
static int audiobuffer_get (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    if (buf == NULL || buf->nchannels <= 0 || buf->nframes <= 0 || lua_gettop(L) < 3) {
        lua_pushnumber (L, 0.0);
        return 1;
    }

    int channel = lua_tointeger (L, 2) - 1;
    int frame   = lua_tointeger (L, 3) - 1;
    lua_pushnumber (L, *(buf->channels[channel] + frame));

    return 1;
}

static int audiobuffer_set (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    if (buf == NULL || lua_gettop (L) < 4)
        return 0;

    int channel = lua_tointeger (L, 2) - 1;
    int frame   = lua_tointeger (L, 3) - 1;
    *(buf->channels[channel] + frame) = (lrt_sample_t) lua_tonumber (L, 4);
    buf->cleared = false;
    return 0;
}

//=============================================================================
static int audiobuffer_array (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushlightuserdata (L, buf->channels);
    return 1;
}

static int audiobuffer_raw (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushlightuserdata (L, buf->channels[lua_tointeger(L, 2)]);
    return 1;
}

static int audiobuffer_applygain (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    int chan  = lua_tointeger (L, 2);
    int start = lua_tointeger (L, 3);
    int count = lua_tointeger (L, 4);
    lua_Number gain  = lua_tonumber (L, 5);
    lrt_sample_t* d = buf->channels [chan];
    for (int i = start; count >= 0; ++i) {
        d[i] = d[i] * gain;
        --count;
    }
    return 0;
}

static int audiobuffer_referto (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lrt_sample_t* const* data = lua_touserdata (L, 2);
    int nchannels  = MAX (0, lua_tointeger (L, 3));
    int nframes    = MAX (0, lua_tointeger (L, 4));
    lrt_audiobuffer_referto (buf, data, nchannels, nframes);
    return 0;
}

static int audiobuffer_tostring (lua_State* L) {
    lua_pushstring (L, "lrt.audio.Buffer");
    return 1;
}

static int audiobuffer_handle (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, lua_upvalueindex (1));
    lua_pushlightuserdata (L, buf);
    return 1;
}

static int f_round (lua_State* L) {
    lua_pushnumber (L, (lua_Number)(lrt_sample_t) lua_tonumber (L, 1));
    return 1;
}

#define MINUS_INFINITY_DB   -100.0
#define UNITY_GAIN          1.0

static int f_gain2db (lua_State* L) {
    int isnum = 0;
    lua_Number gain = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) gain = UNITY_GAIN;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = MINUS_INFINITY_DB;
    lua_pushnumber (L, gain > 0.0 ? fmax (infinity, log10 (gain) * 20.0) : infinity);
    return 1;
}

static int f_dbtogain (lua_State* L) {
    int isnum = 0;
    lua_Number db = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) db = 1.0;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = MINUS_INFINITY_DB;
    lua_pushnumber (L, db > infinity ? pow (10.f, db * 0.05) : 0.0);
    return 1;
}

//=============================================================================
static const luaL_Reg audio_f[] = {
    { "Buffer",     audiobuffer_new },
    { "round",      f_round },
    { "db2gain",    f_dbtogain },
    { "gain2db",    f_gain2db },
    { NULL, NULL }
};

static const luaL_Reg audiobuffer_mm[] = {
    { "__gc",   audiobuffer_gc },
    { NULL, NULL }
};

static const luaL_Reg audiobuffer_tbl_mm[] = {
    { "__tostring",   audiobuffer_tostring },
    { NULL, NULL }
};

static const luaL_Reg audiobuffer_m[] = {
    { "array",      audiobuffer_array },
    { "raw",        audiobuffer_raw },
    { "channels",   audiobuffer_channels },
    { "length",     audiobuffer_length },
    { "clear",      audiobuffer_clear },
    { "cleared",    audiobuffer_cleared },
    { "get",        audiobuffer_get },
    { "set",        audiobuffer_set },
    { "applygain",  audiobuffer_applygain },
    { "handle",     audiobuffer_handle },
    { NULL, NULL }
};

lrt_audio_buffer_t* lrt_audio_buffer_handle (lua_State* L, int index) {
    lua_getfield (L, index, "handle");
    lua_call (L, 0, 1);
    lrt_audio_buffer_t* buf = lua_touserdata (L, -1);
    lua_pop (L, -1);
    return buf;
}

int lrt_audio_buffer_channels (lrt_audio_buffer_t* buf) {
    return buf->nchannels;
}

int lrt_audio_buffer_length (lrt_audio_buffer_t* buf) {
    return buf->nframes;
}

void lrt_audio_buffer_refer_to (lrt_audio_buffer_t* buffer, float* const* data, int nchans, int nframes) {
    lrt_audiobuffer_referto (buffer, data, nchans, nframes);
}

lrt_sample_t** lrt_audio_buffer_array (lrt_audio_buffer_t* buf) {
    return buf->channels;
}

static int audiobuffer_new (lua_State* L) {
    lua_newtable (L);
    luaL_getmetatable (L, MT_AUDIO_BUFFER);
    lua_setmetatable (L, -2);

    AudioBuffer* buf = lua_newuserdata (L, sizeof (AudioBuffer));
    luaL_getmetatable (L, MT_AUDIO_BUFFER_DATA);
    lua_setmetatable (L, -2);

    audiobuffer_init (buf);
    if (lua_gettop (L) >= 2 && lua_isnumber (L, 1) && lua_isnumber (L, 2)) {
        buf->nchannels = (int) MAX (1.0, lua_tonumber (L, 1));
        buf->nframes   = (int) MAX (1.0, lua_tonumber (L, 2));
        audiobuffer_allocate_data (buf);
    }

    luaL_setfuncs (L, audiobuffer_m, 1);

    return 1;
}

int lrt_audio_bufer_new (lua_State* L, int nchans, int nframes) {
    int top = lua_gettop (L);
    lua_pushinteger (L, MAX (0, nchans));
    lua_pushinteger (L, MAX (0, nframes));
    int res = audiobuffer_new (L);
    lua_remove (L, top + 1);
    lua_remove (L, top + 1);
    return res;
}

int luaopen_audio (lua_State* L) {
    luaL_newmetatable (L, MT_AUDIO_BUFFER_DATA);
    luaL_setfuncs (L, audiobuffer_mm, 0);
    lua_pop (L, -1);

    luaL_newmetatable (L, MT_AUDIO_BUFFER);
    luaL_setfuncs (L, audiobuffer_tbl_mm, 0);
    lua_pop (L, -1);

    luaL_newlib (L, audio_f);
    return 1;
}
