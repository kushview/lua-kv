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

#ifndef LRT_AUDIO_H
#define LRT_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <lua.h>

// typedef lua_Number                          lrt_sample_t;
typedef float                               lrt_sample_t;
typedef struct lrt_audio_buffer_impl_t      lrt_audio_buffer_t;
typedef struct lrt_midi_message_impl_t      lrt_midi_message_t;
typedef struct lrt_midi_buffer_impl_t       lrt_midi_buffer_t;
typedef void*                               lrt_midi_buffer_iter_t;

/** Adds a new audio buffer to the lua stack
    @param L            The lua state
    @param nchannels    Total audio channels
    @param nframes      Number of samples in each channel
*/
int lrt_audio_bufer_new (lua_State* L, int nchannels, int nframes);

/** Returns the underlying C-object 
    @param L        The lua state
    @param index    Index of the buffer on the stack
*/
lrt_audio_buffer_t* lrt_audio_buffer_handle (lua_State* L, int index);

/** Refer the given buffer to a set of external audio channels */
void lrt_audio_buffer_refer_to (lrt_audio_buffer_t* buffer,
                                lrt_sample_t* const* data,
                                int num_channels,
                                int num_frames);

/** Returs this buffer's channel cound */
int lrt_audio_buffer_channels (lrt_audio_buffer_t*);

/** Returns this buffer's length in samples */
int lrt_audio_buffer_length (lrt_audio_buffer_t*);

/** Returns an array of channels. DO NOT keep a reference to this */
lrt_sample_t** lrt_audio_buffer_array (lrt_audio_buffer_t*);

lrt_midi_buffer_t* lrt_midi_buffer_new (size_t size);

void lrt_midi_buffer_free (lrt_midi_buffer_t*);
void lrt_midi_buffer_free_data (lrt_midi_buffer_t*);
void lrt_midi_buffer_insert (lrt_midi_buffer_t* buf, uint8_t* bytes, size_t len, int frame);
lrt_midi_buffer_iter_t lrt_midi_buffer_begin (lrt_midi_buffer_t*);
lrt_midi_buffer_iter_t lrt_midi_buffer_end (lrt_midi_buffer_t*);
lrt_midi_buffer_iter_t lrt_midi_buffer_next (lrt_midi_buffer_t*, lrt_midi_buffer_iter_t);

#define lrt_midi_buffer_foreach(b, i) \
for (lrt_midi_buffer_iter_t (i) = lrt_midi_buffer_begin ((b)); \
    i < lrt_midi_buffer_end ((b)); \
    i = lrt_midi_buffer_next ((b), (i)))
#define lrt_midi_buffer_iter_frame(i)   *(int32_t*)i
#define lrt_midi_buffer_iter_size(i)    (int)(*(uint16_t*)(i + sizeof(int32_t)))
#define lrt_midi_buffer_iter_data(i)    (uint8_t*)i + (sizeof(int32_t) + sizeof(uint16_t))

void lrt_openlibs (lua_State* L);

#ifdef __cplusplus
}
#endif
#endif
