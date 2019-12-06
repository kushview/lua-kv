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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../src/audio.c"
#include "../src/midi.c"
#include "../src/lrtlib.c"

static const int nchannels = 1;
static const int nframes   = 512;

typedef struct _refdata_t {
    int channels;
    int frames;
    float** data;
} refdata_t;

static lua_State* open() {
    lua_State* L = luaL_newstate();
    luaL_openlibs (L);
    lrt_openlibs (L);
    return L;
}

static void close (lua_State* L) {
    lua_close (L);
}

static refdata_t* refdata_new (int nc, int nf) {
    assert (nc > 0 && nf > 0);
    refdata_t* rd = malloc (sizeof (refdata_t));

    rd->channels = nc;
    rd->frames = nf;
    size_t sz = sizeof(float*) * nc + sizeof(float) * nc * nf;
    rd->data = malloc (sz);
    float* ptr = (float*)(rd->data + nc);
    int i;
    for (i = 0; i < nc; ++i)
        rd->data[i] = (ptr + i * nf);
    for (i = 0; i < nc; ++i)
        memset (rd->data[i], 0, sizeof(float) * (size_t)nf);
    return rd;
}

static void refdata_free (refdata_t* data) {
    if (data->data != NULL) {
        free (data->data);
        data->data = NULL;
    }
    free (data);
}

static void lrt_log (const char* msg) {
    printf ("%s\n", msg);
}

static void test_basics (lua_State* L, lrt_audio_buffer_t* buf) {
    lrt_log ("not NULL");
    assert (buf != NULL);

    assert (lrt_audio_buffer_length (buf) == nframes);
    lrt_log ("lrt_audio_buffer_length(): ok");
    
    assert (lrt_audio_buffer_channels (buf) == nchannels);
    lrt_log ("lrt_audio_buffer_channels(): ok");
}

static void test_referto (lua_State* L, lrt_audio_buffer_t* buf) {
    refdata_t* rd = refdata_new (2, 1024);
    assert (rd->frames == 1024);
    assert (rd->channels == 2);
    assert (rd->data != NULL);
    lrt_audio_buffer_refer_to (buf, rd->data, rd->channels, rd->frames);
    assert (lrt_audio_buffer_channels (buf) == rd->channels);
    assert (lrt_audio_buffer_length (buf) == rd->frames);

    lrt_sample_t** arr = lrt_audio_buffer_array (buf);
    for (int i = 0; i < rd->channels; ++i) {
        assert (rd->data[i] == arr[i]);
        for (int f = 0; f < rd->frames; ++f) {
            assert (rd->data[i][f] == arr[i][f]);
        }
    }

    // reset internal buffer data;
    AudioBuffer* ab = (AudioBuffer*) buf;
    ab->channels = (float**) ab->data;

    refdata_free (rd);
}

static void test_foreach() {
    MidiBuffer* buf = lrt_midi_buffer_new (0);
    
    uint8_t data [3];
    data[0] = 0x90;
    data[1] = 100;
    data[2] = 127;
    lrt_midi_buffer_insert (buf, data, 3, 20);
    data[0] = 0x80;
    data[2] = 0;
    lrt_midi_buffer_insert (buf, data, 3, 0);

    data[0] = 0x90;
    data[1] = 50;
    data[2] = 127;
    lrt_midi_buffer_insert (buf, data, 3, 10);
    data[0] = 0x80;
    data[2] = 0;
    lrt_midi_buffer_insert (buf, data, 3, 50);

    lrt_midi_buffer_foreach (buf, i) {
        printf ("frame=%d\n", lrt_midi_buffer_iter_frame (i));
        printf ("len=%d\n", lrt_midi_buffer_iter_size (i));
        uint8_t* data = lrt_midi_buffer_iter_data (i);
        printf ("data1=0x%02x data2=0x%02x data3=0x%02x\n", data[0], data[1], data[2]);
        printf ("\n");
    }
}

static void test_api (lua_State* L) {
    int error = luaL_loadfile (L, "tests/test.lua") || lua_pcall (L, 0, 0, 0);
    if (error) {
        luaL_traceback (L, L, lua_tostring (L, -1), 1);
        lua_pop (L, -2);  /* pop error message from the stack */
        printf("%s\n", lua_tostring (L, -1));
    }
}

static void test_audio (lua_State* L) {
    lrt_audio_bufer_new (L, nchannels, nframes);
    assert (1 == lua_gettop (L));

    lrt_audio_buffer_t* buf = lrt_audio_buffer_handle (L, -1);
    test_basics (L, buf);
    test_referto (L, buf);
    lua_pop (L, -1);
    buf = NULL;
}

int main() {
    lrt_log ("\nTesting C functions");
    lua_State* L = open();
    assert (0 == lua_gettop (L));
    
    test_audio (L);
    test_foreach();
    test_api (L);
    
    close (L);
    L = NULL;
    return 0;
}
