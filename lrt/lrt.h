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

#ifndef LRT_H
#define LRT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
 #define LRT_EXTERN extern "C"
#else
 #define LRT_EXTERN
#endif

#ifdef _WIN32
 #define LRT_EXPORT LRT_EXTERN __declspec(dllexport)
#else
 #define LRT_EXPORT LRT_EXTERN __attribute__((visibility("default")))
#endif

#include <stdbool.h>
#include <stdint.h>
#include <lua.h>

#ifndef LRT_FORCE_FLOAT32
 #define LRT_FORCE_FLOAT32                  0
#endif

#define LRT_MT_AUDIO_BUFFER                 "*lrt_audio_buffer_t"
#define LRT_MT_MIDI_MESSAGE                 "*lrt_midi_message_t"
#define LRT_MT_MIDI_BUFFER                  "*lrt_midi_buffer_t"
#define LRT_MT_MIDI_PIPE                    "*lrt_midi_pipe_t"
#define LRT_MT_VECTOR                       "*lrt_vector_t"

#if LRT_FORCE_FLOAT32
typedef float                               lrt_sample_t;
#else
typedef lua_Number                          lrt_sample_t;
#endif

typedef struct lrt_audio_buffer_impl_t      lrt_audio_buffer_t;
typedef struct lrt_midi_message_impl_t      lrt_midi_message_t;
typedef struct lrt_midi_buffer_impl_t       lrt_midi_buffer_t;
typedef void*                               lrt_midi_buffer_iter_t;
typedef struct lrt_midi_pipe_impl_t         lrt_midi_pipe_t;
typedef struct lrt_vector_impl_t            lrt_vector_t;

/** Creates a new vector leaving it on the stack */
lrt_vector_t* lrt_vector_new (lua_State*, int);

/** Returns the number of elements used by the vector */
size_t lrt_vector_size (lrt_vector_t*);

/** Returns the total number of elements allocated.
    This is NOT the number of elements currently used.
    see `lrt_vector_size`
*/
size_t lrt_vector_capacity (lrt_vector_t*);

/** Clears the vector */
void lrt_vector_clear (lrt_vector_t*);

/** Returns a value from the vector */
lrt_sample_t lrt_vector_get (lrt_vector_t*, int);

/** Sets a values in the vector */
void lrt_vector_set (lrt_vector_t*, int, lrt_sample_t);

/** Resizes the vector.  Does not allocate memory if the new size
    is less than the total capacity. see `lrt_vector_capacity`
*/
void lrt_vector_resize (lrt_vector_t*, int);

//=============================================================================
/** Adds a new audio buffer to the lua stack
    @param L                The lua state
    @param num_channels     Total audio channels
    @param num_frames       Number of samples in each channel
*/
lrt_audio_buffer_t* lrt_audio_buffer_new (lua_State* L,
                                          int        num_channels,
                                          int        num_frames);

/** Refer the given buffer to a set of external audio channels
 
    @param buffer       The audio buffer
    @param data         External data to refer to
    @param nchannels    Number of channels is external data
    @param nframes      Number of samples in each external data channel
*/
void lrt_audio_buffer_refer_to (lrt_audio_buffer_t*  buffer,
                                lrt_sample_t* const* data,
                                int                  nchannels,
                                int                  nframes);

/** Returs this buffer's channel count */
int lrt_audio_buffer_channels (lrt_audio_buffer_t*);

/** Returns this buffer's length in samples */
int lrt_audio_buffer_length (lrt_audio_buffer_t*);

/** Returns an array of channels. DO NOT keep a reference to this. */
lrt_sample_t** lrt_audio_buffer_array (lrt_audio_buffer_t*);

/** Returns a single channel of samples */
lrt_sample_t* lrt_audio_buffer_channel (lrt_audio_buffer_t*, int channel);

/** Resize this buffer

    @param buffer       Buffer to resize
    @param nchannels    New channel count
    @param nframes      New sample count
    @param preserve     Keep existing content if possible
    @param clear        Clear extra space
    @param norealloc    Avoid re-allocating if possible
*/
void lrt_audio_buffer_resize (lrt_audio_buffer_t* buffer,
                              int                 nchannels, 
                              int                 nframes,
                              bool                preserve, 
                              bool                clear,
                              bool                norealloc);

void lrt_audio_buffer_duplicate (lrt_audio_buffer_t*        buffer,
                                 const lrt_sample_t* const* source,
                                 int                        nchannels,
                                 int                        nframes);

void lrt_audio_buffer_duplicate_32 (lrt_audio_buffer_t* buffer,
                                    const float* const* source,
                                    int                 nchannels,
                                    int                 nframes);

//=============================================================================
/** Adds a new midi buffer to the stack */
lrt_midi_buffer_t* lrt_midi_buffer_new (lua_State* L, size_t size);

/** Clears the buffer */
void lrt_midi_buffer_clear (lrt_midi_buffer_t*);

/** Inserts some MIDI data in the buffer */
void lrt_midi_buffer_insert (lrt_midi_buffer_t* buf, const uint8_t* bytes, size_t len, int frame);

/** Returns the start iterator
    Do not modify the retured iterator in any way
*/
lrt_midi_buffer_iter_t lrt_midi_buffer_begin (lrt_midi_buffer_t*);

/** Returns the end iterator
    Do not modify the retured iterator in any way
*/
lrt_midi_buffer_iter_t lrt_midi_buffer_end (lrt_midi_buffer_t*);

/** Returns the next event iterator
    Do not modify the retured iterator in any way
*/
lrt_midi_buffer_iter_t lrt_midi_buffer_next (lrt_midi_buffer_t*, lrt_midi_buffer_iter_t);

/** Loop through all midi events */
#define lrt_midi_buffer_foreach(b, i) \
for (lrt_midi_buffer_iter_t (i) = lrt_midi_buffer_begin ((b)); \
    i < lrt_midi_buffer_end ((b)); \
    i = lrt_midi_buffer_next ((b), (i)))

/** Returns the total size in bytes of the iterator */
#define lrt_midi_buffer_iter_total_size(i) (lua_Integer)(sizeof(int32_t) + sizeof(uint16_t) + *(uint16_t*)((i) + sizeof(int32_t)))

/** Returns the frame index of this event */
#define lrt_midi_buffer_iter_frame(i)   *(int32_t*)i

/** Returns the data size of this event */
#define lrt_midi_buffer_iter_size(i) (lua_Integer)(*(uint16_t*)((uint8_t*)i + sizeof(int32_t)))

/** Returns the raw MIDI data of this event
    Do not modify in any way
*/
#define lrt_midi_buffer_iter_data(i) (uint8_t*)i + (sizeof(int32_t) + sizeof(uint16_t))

//=============================================================================
/** Create a new midi pipe on the stack */
lrt_midi_pipe_t* lrt_midi_pipe_new (lua_State* L, int nbuffers);

/** Clear buffers in the pipe */
void lrt_midi_pipe_clear (lrt_midi_pipe_t*, int);

/** Change the number of buffers contained */
void lrt_midi_pipe_resize (lua_State* L, lrt_midi_pipe_t*, int);

/** Returns a buffer from the list */
lrt_midi_buffer_t* lrt_midi_pipe_get (lrt_midi_pipe_t*, int);

//=============================================================================
/** Open all libraries
    @param L    The Lua state
    @param glb  Set 1 to assign a global variable for each library
*/
void lrt_openlibs (lua_State* L, int glb);

#ifdef __cplusplus
}
#endif
#endif
