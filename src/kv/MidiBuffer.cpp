/// A MIDI buffer.
// Designed for real time performance, therefore does virtually no type
// checking in method calls.
// @classmod kv.MidiBuffer
// @pragma nostrip

#include "kv/lua/midi_buffer.hpp"
#include "packed.h"
#define LKV_MT_MIDI_BUFFER_TYPE "kv.MidiBufferClass"

using MidiBuffer    = juce::MidiBuffer;
using Iterator      = juce::MidiBufferIterator;
using MidiMessage   = juce::MidiMessage;
using Impl          = kv::lua::MidiBufferImpl;

/// Create an empty MIDI Buffer
// @function MidiBuffer.__call
// @return A new midi buffer
// @within Constructors

/// Create a new MIDI Buffer
// @param size Size in bytes
// @function MidiBuffer.__call
// @return A new MIDI Buffer
// @within Constructors
static int midibuffer_new (lua_State* L) {
    auto** impl = kv::lua::new_midibuffer (L);
    if (lua_gettop (L) > 1) {
        if (lua_isinteger (L, 2)) {
            (**impl).buffer.ensureSize (static_cast<size_t> (lua_tointeger (L, 2)));
        }
    }
    return 1;
}

static int midibuffer_free (lua_State* L) {
    auto** impl = (Impl**) lua_touserdata (L, 1);
    if (nullptr != *impl) {
        (*impl)->free (L);
        delete (*impl);
        *impl = nullptr;
    }
    return 0;
}

/// Reserve an amount of space.
// @param size Size in bytes to reserve
// @function MidiBuffer:reserve
// @return Size reserved in bytes or false
static int midibuffer_reserve (lua_State* L) {
    if (auto* impl = *(Impl**) lua_touserdata (L, 1)) {
        auto size = lua_tointeger (L, 2);
        (*impl).buffer.ensureSize (static_cast<size_t> (size));
        lua_pushinteger (L, size);
    } else {
        lua_pushboolean (L, false);
    }
    return 1;
}

//==============================================================================
static int midibuffer_events_closure (lua_State* L) {
    auto* impl = (Impl*) lua_touserdata (L, lua_upvalueindex (1));
    
    if (impl->iter == impl->buffer.end()) {
        lua_pushnil (L);
        return 1;
    }
    
    const auto& ref = (*(*impl).iter);
    lua_pushlightuserdata (L, (void*) ref.data);
    lua_pushinteger (L, ref.numBytes);
    lua_pushinteger (L, ref.samplePosition);
    ++impl->iter;

    return 3;
}

/// Iterate over MIDI data.  
// Iterate over messages in this buffer
// @function MidiBuffer:events
// @return Event data iterator
// @usage
// -- @data    Raw bite array
// -- @size    Size in bytes
// -- @frame   Audio frame index in buffer
// for data, size, frame in buffer:iter() do
//     -- do something with midi data
// end
static int midibuffer_events (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->reset_iter();
    lua_pushlightuserdata (L, impl);
    lua_pushcclosure (L, midibuffer_events_closure, 1);
    return 1;
}

//==============================================================================
static int midibuffer_messages_closure (lua_State* L) {
    auto* impl = (Impl*) lua_touserdata (L, lua_upvalueindex (1));
    if (impl->iter == impl->buffer.end()) {
        lua_pushnil (L);
        return 1;
    }

    const auto& ref = (*(*impl).iter);
    **impl->message = ref.getMessage();
    lua_rawgeti (L, LUA_REGISTRYINDEX, impl->msgref);
    lua_pushinteger (L, ref.samplePosition);
    ++impl->iter;

    return 2;
}

/// Iterate over MIDI Event.  
// Iterate over messages (kv.MidiMessage) in the buffer
// @function MidiBuffer:messages
// @return message iterator
// @usage
// -- @msg     A kv.MidiMessage
// -- @size    Size in bytes
// -- @frame   Audio frame index in buffer
// for msg, frame in buffer:events() do
//     -- do something with midi data
// end
static int midibuffer_messages (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->reset_iter();
    lua_pushlightuserdata (L, impl);
    lua_pushcclosure (L, midibuffer_messages_closure, 1);
    return 1;
}

//==============================================================================
/// Add a message to the buffer
// @function MidiBuffer:add_message
// @tparam kv.MidiMessage msg Message to add
// @int frame Insert index
static int midibuffer_add_message (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.addEvent (
        **(MidiMessage**) lua_touserdata (L, 2), 
        lua_tointeger (L, 3));
    return 0;
}

/// Add a raw MIDI Event
// @function MidiBuffer:add_event
// @param data Message to add
// @int size Size of data in bytes
// @int frame Insert index
static int midibuffer_add_event (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.addEvent ((juce::uint8*) lua_touserdata (L, 2),
                            lua_tointeger (L, 3), lua_tointeger (L, 4));
    return 0;
}

/// Exchanges the contents of this buffer with another one.
// This is a quick operation, because no memory allocating or copying is done, it
// just swaps the internal state of the two buffers.
// @function MidiBuffer.swap
// @tparam kv.MidiBuffer other Buffer to swap with
static int midibuffer_swap (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    impl->buffer.swapWith (**(MidiBuffer**) lua_touserdata (L, 2));
    return 0;
}

/// Removes all events from the buffer
// @function MidiBuffer.clear
static int midibuffer_clear (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    (*impl).buffer.clear();
    return 0;
}

// Removes all events between two times from the buffer.
// All events for which (start <= event position < start + numSamples) will
// be removed.
// @function MidiBuffer.clear_range
// @param start
// @param last
static int midibuffer_clear_range (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    (*impl).buffer.clear (lua_tointeger (L, 2),
                          lua_tointeger (L, 3));
    return 0;
}

static int midibuffer_num_events (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    lua_pushinteger (L, (*impl).buffer.getNumEvents());
    return 1;
}

static int midibuffer_add_buffer (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    if (lua_gettop (L) >= 5) {
        impl->buffer.addEvents (
            **(MidiBuffer**) lua_touserdata (L, 2),
            lua_tointeger (L, 3),
            lua_tointeger (L, 4),
            lua_tointeger (L, 5));
    } else {
        lua_error (L);
    }
    return 0;
}

static int midibuffer_insert (lua_State* L) {
    auto* impl = *(Impl**) lua_touserdata (L, 1);
    kv_packed_t pack = { .packed = lua_tointeger (L, 2) };
    impl->buffer.addEvent ((uint8_t*) pack.data, 4, lua_tointeger (L, 3));
    return 0;
}

//==============================================================================
static const luaL_Reg buffer_methods[] = {
    { "__gc",               midibuffer_free },
    // { "__tostring",         midibuffer_tostring },
    { "clear",              midibuffer_clear },
    { "clear_range",        midibuffer_clear_range },
    { "num_events",         midibuffer_num_events },
    { "reserve",            midibuffer_reserve },
    { "swap",               midibuffer_swap },

    { "insert",             midibuffer_insert },

    { "events",             midibuffer_events },
    { "add_event",          midibuffer_add_event },

    { "messages",           midibuffer_messages },
    { "add_message",        midibuffer_add_message },

    { "add_buffer",         midibuffer_add_buffer },
    { NULL, NULL }
};

//==============================================================================
LUAMOD_API
int luaopen_kv_MidiBuffer (lua_State* L) {
    if (luaL_newmetatable (L, LKV_MT_MIDI_BUFFER)) {
        lua_pushvalue (L, -1);               /* duplicate the metatable */
        lua_setfield (L, -2, "__index");     /* mt.__index = mt */
        luaL_setfuncs (L, buffer_methods, 0);
        lua_pop (L, 1);
    }

    if (luaL_newmetatable (L, LKV_MT_MIDI_BUFFER_TYPE)) {
        lua_pushcfunction (L, midibuffer_new);   /* push audio_new function */
        lua_setfield (L, -2, "__call");     /* mt.__call = audio_new */
        lua_pop (L, 1);
    }

    lua_newtable (L);
    luaL_setmetatable (L, LKV_MT_MIDI_BUFFER_TYPE);
    return 1;
}
