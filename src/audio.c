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

/// Audio classes and utilities
// @author Michael Fisher
// @module kv.audio

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#ifdef _MSC_VER
 #include <cstdalign>
#else
 #include <stdalign.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "luainc.h"
#include "vector.h"
#include "util.h"
#include "lua-kv.h"

#define LKV_MINUS_INFINITY_DB   -100.0
#define LKV_UNITY_GAIN          1.0
#define LKV_NPREALLOC           32

struct kv_audio_buffer_impl_t {
    lua_State* L;
    int nframes;                            // sample count
    int nchannels;                          // channel count
    kv_vector_t** vectors;
    int* vecrefs;
    size_t size;
    char* data;
    kv_sample_t** channels;                // channels actually used
    kv_sample_t* prealloc [LKV_NPREALLOC]; // pre-allocated channel space
    bool cleared;                          // true if buffer has been cleared
};

typedef struct kv_audio_buffer_impl_t  AudioBuffer;

//=============================================================================
static void kv_audio_buffer_init (AudioBuffer* buf) {
    buf->nframes = buf->nchannels = 0;
    buf->size = 0;
    buf->data = NULL;
    buf->vecrefs = NULL;
    buf->vectors = NULL;
    memset (buf->prealloc, 0, sizeof(kv_sample_t*) * LKV_NPREALLOC);
    buf->channels = (kv_sample_t**) buf->prealloc;
    buf->cleared = false;
}

static void kv_audio_buffer_alloc_channels (AudioBuffer* buf, kv_sample_t* const* data, int offset) {
    assert (offset >= 0);
    
    if (buf->nchannels < LKV_NPREALLOC) {
        buf->channels = (kv_sample_t**) buf->prealloc;
    } else {
        buf->size = (size_t)(buf->nchannels + 1) * sizeof (kv_sample_t*);
        buf->data = malloc (buf->size);
        buf->channels = (kv_sample_t**)(buf->data);
    }

    if (data != NULL) {
        for (int i = 0; i < buf->nchannels; ++i) {
            assert (data[i] != NULL);
            buf->channels[i] = data[i] + offset;
        }
    }

    buf->channels [buf->nchannels] = NULL;
    buf->cleared = false;
}

static void kv_audio_buffer_free_vectors (lua_State* L, kv_audio_buffer_t* buf) {
    if (buf->vectors == NULL || buf->vecrefs == NULL) {
        return;
    }
    
    for (int c = 0; c < buf->nchannels; ++c) {
        buf->vectors[c] = NULL;
        luaL_unref (L, LUA_REGISTRYINDEX, buf->vecrefs[c]);
        buf->vecrefs[c] = LUA_NOREF;
    }

    free (buf->vectors);
    buf->vectors = NULL;
    free (buf->vecrefs);
    buf->vecrefs = NULL;
}

static void kv_audio_buffer_free_data (AudioBuffer* buf);

static void kv_audio_buffer_create_vectors (lua_State* L, kv_audio_buffer_t* buf) {
    kv_audio_buffer_free_vectors (L, buf);
    kv_audio_buffer_free_data (buf);
    kv_audio_buffer_alloc_channels (buf, NULL, 0);

    buf->vectors = malloc (sizeof (kv_vector_t*) * (buf->nchannels + 1));
    buf->vecrefs = malloc (sizeof (int) * (buf->nchannels + 1));

    int c;
    for (c = 0; c < buf->nchannels; ++c) {
        buf->vectors[c]  = kv_vector_new (L, buf->nframes);
        buf->channels[c] = kv_vector_values (buf->vectors [c]);
        buf->vecrefs[c]  = luaL_ref (L, LUA_REGISTRYINDEX);
    }

    buf->vectors[buf->nchannels] = NULL;
    buf->vecrefs[buf->nchannels] = LUA_REFNIL;

    for (c = buf->nchannels; c < LKV_NPREALLOC; ++c) {
        buf->prealloc[c] = NULL;
    }
}

kv_audio_buffer_t* kv_audio_buffer_new (lua_State* L,
                                          int        nchans, 
                                          int        nframes)
{
    AudioBuffer* buf = lua_newuserdata (L, sizeof (AudioBuffer));
    luaL_setmetatable (L, LKV_MT_AUDIO_BUFFER);
    buf->L = L;
    kv_audio_buffer_init (buf);

    if (nchans > 0 && nframes > 0) {
        buf->nchannels = nchans;
        buf->nframes   = nframes;
        kv_audio_buffer_create_vectors (L, buf);
    }

    return buf;
}

static void kv_audio_buffer_free_data (AudioBuffer* buf) {
    if (buf->size != 0 && buf->data != NULL) {
        free (buf->data);
        buf->size = 0;
        buf->data = NULL; 
    }
}

int kv_audio_buffer_channels (kv_audio_buffer_t* buf) {
    return buf->nchannels;
}

int kv_audio_buffer_length (kv_audio_buffer_t* buf) {
    return buf->nframes;
}

void kv_audio_buffer_refer_to (kv_audio_buffer_t*  buf, 
                                kv_sample_t* const* data, 
                                int                  nchannels, 
                                int                  nframes) 
{
    if (buf->size != 0)
        kv_audio_buffer_free_data (buf);
    buf->nchannels = nchannels;
    buf->nframes   = nframes;
    kv_audio_buffer_alloc_channels (buf, data, 0);
}

kv_sample_t** kv_audio_buffer_array (kv_audio_buffer_t* buf) {
    return buf->channels;
}

kv_sample_t* kv_audio_buffer_channel (kv_audio_buffer_t* buf, int channel) {
    return buf->channels [channel];
}

void kv_audio_buffer_resize (kv_audio_buffer_t* buf,
                              int                 nchannels, 
                              int                 nframes,
                              bool                preserve, 
                              bool                clear,
                              bool                norealloc)
{
    if (nchannels == buf->nchannels && nframes == buf->nframes) {
        return;
    }

    kv_audio_buffer_free_vectors (buf->L, buf);
    buf->nchannels = nchannels;
    buf->nframes   = nframes;
    kv_audio_buffer_create_vectors (buf->L, buf);
}

void kv_audio_buffer_duplicate (kv_audio_buffer_t*        buffer,
                                 const kv_sample_t* const* source,
                                 int                        nchannels,
                                 int                        nframes)
{
    kv_audio_buffer_resize (buffer, nchannels, nframes, false, false, true);

    for (int c = 0; c < nchannels; ++c) {
        memcpy (buffer->channels[c], source[c], sizeof(kv_sample_t) * nframes);
    }
}

void kv_audio_buffer_duplicate_32 (kv_audio_buffer_t* buffer,
                                    const float* const* source,
                                    int                 nchannels,
                                    int                 nframes)
{
    kv_audio_buffer_resize (buffer, nchannels, nframes, false, false, true);

    for (int c = 0; c < nchannels; ++c) {
        const float*  src = source [c];
        kv_sample_t* dst = buffer->channels [c];
        for (int f = 0; f < nframes; ++f) {
            dst[f] = (kv_sample_t) src[f];
        }
    }
}

//=============================================================================
static void kv_audio_buffer_clear_all (AudioBuffer* buf) {
    if (buf->cleared)
        return;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c], 0, sizeof(kv_sample_t) * (size_t)buf->nframes);
    buf->cleared = true;
}

static void kv_audio_buffer_clear_range (AudioBuffer* buf, int start, int count) {
    if (buf->cleared)
        return;
    if (start == 0 && count == buf->nframes)
        buf->cleared = true;
    for (int c = 0; c < buf->nchannels; ++c)
        memset (buf->channels[c] + start, 0, sizeof(kv_sample_t) * (size_t)buf->nframes);
}

static void kv_audio_buffer_clear_channel_range (AudioBuffer* buf, int channel, int start, int count) {
    if (buf->cleared)
        return;
    memset (buf->channels[channel] + start, 0, sizeof(kv_sample_t) * count);
}

static void kv_audio_buffer_clear_channel (AudioBuffer* buf, int channel) {
    kv_audio_buffer_clear_channel_range (buf, channel, 0, buf->nframes);
    if (channel == 0 && buf->nchannels == 1)
        buf->cleared = true;
}

//=============================================================================

/// Creates a empty audio buffer.
// @function Buffer
// @return An empty buffer.

/// Creates a new audio buffer with the specified channel and sample counts
// @param nchannels Number of channels
// @param nframes Number of samples in each channel
// @function Buffer
// @return Newly created audio buffer
static int audiobuffer_new (lua_State* L) {
    int nchans = 0, nframes = 0;
    if (lua_gettop (L) >= 2 && lua_isinteger (L, 1) && lua_isinteger (L, 2)) {
        nchans  = (int) MAX (0, lua_tointeger (L, 1));
        nframes = (int) MAX (0, lua_tointeger (L, 2));
    }
    kv_audio_buffer_new (L, nchans, nframes);
    return 1;
}

/// An audio buffer
// @type Buffer

static int audiobuffer_gc (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    kv_audio_buffer_free_data (buf);
    buf->L = NULL;
    return 0;
}

/// Clears the entire audio buffer
// @function clear

/// Clears one channel in the audio buffer
// @int channel The channel to clear
// @function clear

/// Clears a range of samples on all channels
// @int start Sample index to start on
// @int count Number of samples to clear
// @function clear

/// Clears a range of samples on one channel
// @int channel The channel to clear
// @int start Sample index to start on
// @int count Number of samples to clear
// @function clear
static int audiobuffer_clear (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    if (buf == NULL)
        return 0;

    switch (lua_gettop (L)) {
        case 0: break;
        case 1: kv_audio_buffer_clear_all (buf); break;
        
        case 2: {
            kv_audio_buffer_clear_channel (buf, lua_tointeger (L, 2));
        } break;
        
        case 3: {
            kv_audio_buffer_clear_range (buf,
                lua_tointeger (L, 2), 
                lua_tointeger (L, 3));
        } break;
        
        default:
        case 4: {
            kv_audio_buffer_clear_channel_range (buf,
                lua_tointeger (L, 2),
                lua_tointeger (L, 3),
                lua_tointeger (L, 4));
        } break;
    }

    return 0;
}

/// Returns true if the buffer has been cleared
// @function cleared
// @return true if already cleared
static int audiobuffer_cleared (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushboolean (L, buf != NULL ? buf->cleared : false);
    return 1;
}

/// returns the number of samples in the buffer
// @function length
// @return size of the buffer in samples
static int audiobuffer_length (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushinteger (L, buf != NULL ? buf->nframes : 0);
    return 1;
}

/// Channel count
// @function channels
// @return the number of channels held in the buffer
static int audiobuffer_channels (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushinteger (L, buf != NULL ? buf->nchannels : 0);
    return 1;
}

/// Get a single value from the buffer
// @param channel The channel to get from
// @param frame The sample index to get
// @function get
// @return the number of channels held in the buffer
static int audiobuffer_get (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    if (buf == NULL || buf->nchannels <= 0 || buf->nframes <= 0 || lua_gettop(L) < 3) {
        lua_pushnumber (L, 0.0);
        return 1;
    }

    lua_Integer channel = lua_tointeger (L, 2) - 1;
    lua_Integer frame   = lua_tointeger (L, 3) - 1;
    lua_pushnumber (L, *(buf->channels[channel] + frame));

    return 1;
}

/// Get the sample vector for a channel
// @param channel The channel to get
// @function vector
static int audiobuffer_vector (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_Integer i = lua_tointeger (L, 2) - 1;
    if (i >= 0 && i < buf->nchannels) {
        lua_rawgeti (L, LUA_REGISTRYINDEX, buf->vecrefs[i]);
    } else {
        lua_pushnil (L);
    }
    return 1;
}

/// Set a single sample in the buffer
// @param channel The channel to set on
// @param frame The frame index to set
// @param value The value to set
// @function set
static int audiobuffer_set (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    if (buf == NULL || lua_gettop (L) < 4)
        return 0;

    lua_Integer channel = lua_tointeger (L, 2) - 1;
    lua_Integer frame   = lua_tointeger (L, 3) - 1;
    *(buf->channels[channel] + frame) = (kv_sample_t) lua_tonumber (L, 4);
    buf->cleared = false;
    return 0;
}

/// Get raw multi channel data
// @function array
// @return Raw C array of channel data as lightuserdata
static int audiobuffer_array (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushlightuserdata (L, buf->channels);
    return 1;
}

/// Get raw single channel data
// @param channel The channel to get
// @function raw
// @return Raw data for a single channel as lightuserdata
static int audiobuffer_raw (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushlightuserdata (L, buf->channels[lua_tointeger(L, 2)]);
    return 1;
}

/// Apply gain to all channels and samples
// @number gain Gain to apply to all channels
// @function applygain

/// Apply gain to a single channel
// @int channel Channel to apply gain to
// @number gain Gain to apply to a channel
// @function applygain

/// Apply gain to a single channel with range
// @int channel Channel to apply gain to
// @int start Sample index to start at
// @int count Number of samples to process
// @number gain Amount of gain to apply
// @function applygain
static int audiobuffer_applygain (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    switch (lua_gettop (L)) {
        case 2: {
            lua_Number gain = lua_tonumber (L, 2);
            for (lua_Integer c = 0; c < buf->nchannels; ++c) {
                kv_sample_t* d = buf->channels [c];
                for (lua_Integer f = 0; f < buf->nframes; ++f) {
                    d[f] = gain * d[f];
                }
            }
            break;
        }
        case 3: {
            lua_Integer chan = lua_tointeger (L, 2) - 1;
            lua_Number gain  = lua_tonumber (L, 3);
            if (chan >= 0 && chan < buf->nchannels) {
                kv_sample_t* d = buf->channels [chan];
                for (lua_Integer f = 0; f < buf->nframes; ++f) {
                    d[f] = gain * d[f];
                }
            }
            break;
        }
        case 5: {
            lua_Integer chan  = lua_tointeger (L, 2) - 1;
            lua_Integer start = lua_tointeger (L, 3) - 1;
            lua_Integer count = lua_tointeger (L, 4);
            lua_Number gain = lua_tonumber (L, 5);
            kv_sample_t* d = buf->channels [chan];
            for (lua_Integer f = start; --count >= 0; ++f) {
                d[f] = gain * d[f];
            }
            break;
        }
    }

    return 0;
}

/// Fade the buffer from starting gain to ending gain
// @number startgain Starting gain
// @number endgain End gain
// @function fade

/// Fade the buffer from starting gain to ending gain
// @int channel Channel to apply to
// @int start Sample to start at
// @int count Number of samples to process
// @number startgain Starting gain
// @number endgain End gain
// @function fade
static int audiobuffer_fade (lua_State* L) {
    AudioBuffer* buf  = lua_touserdata (L, 1);
    switch (lua_gettop (L)) {
        case 3: {
            // apply gain to all channels/frames
            lua_Number gain1 = lua_tonumber (L, 2);
            lua_Number gain2 = lua_tonumber (L, 3);
            lua_Number inc = buf->nframes > 0 ? (gain2 - gain1) / (lua_Number)buf->nframes
                                              : 0.0;
            lua_Number gain;

            for (lua_Integer c = 0; c < buf->nchannels; ++c) {
                gain = gain1;
                kv_sample_t* d = buf->channels [c];
                for (lua_Integer f = 0; f < buf->nframes; ++f) {
                    d[f] = gain * d[f];
                    gain += inc;
                }
            }
            break;
        }

        case 6: {
            // apply fade to specific channel with start and frame count
            lua_Integer chan  = lua_tointeger (L, 2) - 1;
            lua_Integer start = lua_tointeger (L, 3) - 1;
            lua_Integer count = lua_tointeger (L, 4);
            lua_Number gain1  = lua_tonumber (L, 5);
            lua_Number gain2  = lua_tonumber (L, 6);
            kv_sample_t* d = buf->channels [chan];
            lua_Number inc = count > 0 ? (gain2 - gain1) / (lua_Number) count
                                       : 0.0;
            for (lua_Integer i = start; --count >= 0; ++i) {
                d[i] = gain1 * d[i];
                gain1 += inc;
            }
            break; 
        }
    }

    return 0;
}

/// Refer internal data to another source
// @param data Data to refer to
// @param nchannels Number of channels in new source
// @param nframes Number of samples in new source
// @function referto
static int audiobuffer_referto (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    kv_sample_t* const* data = lua_touserdata (L, 2);
    lua_Integer nchannels  = MAX (0, lua_tointeger (L, 3));
    lua_Integer nframes    = MAX (0, lua_tointeger (L, 4));
    kv_audio_buffer_refer_to (buf, data, nchannels, nframes);
    return 0;
}

/// @function __tostring
static int audiobuffer_tostring (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    lua_pushfstring (L, "AudioBuffer: channels=%d length=%d",
                         buf->nchannels, buf->nframes);
    return 1;
}

static int audiobuffer_apply (lua_State* L) {
    AudioBuffer* buf = lua_touserdata (L, 1);
    if (LUA_TFUNCTION == lua_type (L, 2)) {
        // lua_pushvalue (L, 2);
        // lua_call ()
    }
    return 0;
}

static const luaL_Reg audiobuffer_m[] = {
    { "__gc",       audiobuffer_gc },
    { "__tostring", audiobuffer_tostring },
    { "array",      audiobuffer_array },
    { "raw",        audiobuffer_raw },
    { "channels",   audiobuffer_channels },
    { "length",     audiobuffer_length },
    { "clear",      audiobuffer_clear },
    { "cleared",    audiobuffer_cleared },
    { "get",        audiobuffer_get },
    { "set",        audiobuffer_set },
    { "applygain",  audiobuffer_applygain },
    { "apply",      audiobuffer_apply },
    { "vector",     audiobuffer_vector },
    { "fade",       audiobuffer_fade },
    { NULL, NULL }
};

//=============================================================================
static int f_round (lua_State* L) {
    lua_pushnumber (L, (lua_Number)(kv_sample_t) lua_tonumber (L, 1));
    return 1;
}

static int af_gain2db (lua_State* L) {
    int isnum = 0;
    lua_Number gain = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) gain = LKV_UNITY_GAIN;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = LKV_MINUS_INFINITY_DB;
    lua_pushnumber (L, gain > 0.0 ? fmax (infinity, log10 (gain) * 20.0) : infinity);
    return 1;
}

static int af_dbtogain (lua_State* L) {
    int isnum = 0;
    lua_Number db = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0) db = 1.0;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0) infinity = LKV_MINUS_INFINITY_DB;
    lua_pushnumber (L, db > infinity ? pow (10.f, db * 0.05) : 0.0);
    return 1;
}

static const luaL_Reg audio_f[] = {
    { "Buffer",     audiobuffer_new },
    { "round",      f_round },
    { "db2gain",    af_dbtogain },
    { "dbtogain",   af_dbtogain },
    { "gain2db",    af_gain2db },
    { "gaintodb",   af_gain2db },
    { NULL, NULL }
};

//=============================================================================

LKV_EXPORT int luaopen_kv_audio (lua_State* L) {
    luaL_newmetatable (L, LKV_MT_AUDIO_BUFFER);
    lua_pushvalue (L, -1);               /* duplicate the metatable */
    lua_setfield (L, -2, "__index");     /* mt.__index = mt */
    luaL_setfuncs (L, audiobuffer_m, 0);

    kv_vector_metatable (L);

    luaL_newlib (L, audio_f);
    return 1;
}
