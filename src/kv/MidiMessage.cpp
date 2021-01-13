/// A MIDI Message.
// @classmod kv.MidiMessage
// @pragma nostrip

#include "lua-kv.hpp"
#include "packed.h"
#include LKV_JUCE_HEADER

#define LKV_MT_MIDI_MESSAGE_TYPE "kv.MidiMessageClass"

static auto create_message (lua_State* L) {
    auto** userdata = (juce::MidiMessage**) lua_newuserdata (L, sizeof (juce::MidiMessage**));
    *userdata = new juce::MidiMessage();
    luaL_setmetatable (L, LKV_MT_MIDI_MESSAGE);
    return userdata;
}

static int midimessage_new (lua_State* L) {
    auto** msg = create_message (L);
    if (lua_gettop (L) > 1 && lua_isinteger (L, 2)) {
        kv_packed_t pack = { .packed = lua_tointeger (L, 2) };
        **msg = juce::MidiMessage (pack.data[0], pack.data[1], pack.data[2]);
    }
    return 1;
}

static int midimessage_free (lua_State* L) {
    auto** userdata = (juce::MidiMessage**) lua_touserdata (L, 1);
    if (nullptr != *userdata) {
        delete (*userdata);
        *userdata = nullptr;
    }
    return 0;
}

#define midimessage_get_string(f, m) \
static int midimessage_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    lua_pushstring (L, msg->m().toRawUTF8()); \
    return 1; \
}

#define midimessage_get_number(f, m) \
static int midimessage_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    lua_pushnumber (L, static_cast<lua_Number> (msg->m())); \
    return 1; \
}

#define midimessage_set_float(f, m) \
static int midimessage_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    msg->m (static_cast<lua_Number> (lua_tonumber (L, 2))); \
    return 0; \
}

#define midimessage_get_int(f, m) \
static int midimessage_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    lua_pushinteger (L, msg->m()); \
    return 1; \
}

#define midimessage_set_int(f, m) \
static int midimessage_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    msg->m (lua_tointeger (L, 2)); \
    return 0; \
}

#define midimessage_is(f, m) \
static int midimessage_is_##f (lua_State* L) { \
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
    lua_pushboolean (L, msg->m()); \
    return 1; \
}

midimessage_get_string (description, getDescription)

midimessage_get_number (time, getTimeStamp)
midimessage_set_float (set_time, setTimeStamp)
midimessage_set_float (add_time, addToTimeStamp)

static int midimessage_with_time (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    auto** ret = create_message (L);
    (**ret).setTimeStamp (lua_tonumber (L, 2));
    return 1;
}

midimessage_get_int (channel,       getChannel)
midimessage_set_int (set_channel,   setChannel)
static int midimessage_is_channel (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushboolean (L, msg->isForChannel (lua_tointeger (L, 2)));
    return 1;
}

midimessage_is (note_on,        isNoteOn)
midimessage_is (note_off,       isNoteOff)
midimessage_is (note,           isNoteOnOrOff)
midimessage_get_int (note,      getNoteNumber)
midimessage_set_int (set_note,  setNoteNumber)

static int midimessage_data (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getRawData());
    lua_pushinteger (L, msg->getRawDataSize());
    return 2;
}

midimessage_is (sysex, isSysEx)
static int midimessage_sysex_data (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getSysExData());
    lua_pushinteger (L, msg->getSysExDataSize());
    return 2;
}

midimessage_get_int   (velocity,        getVelocity)
midimessage_get_number (velocity_float,  getFloatVelocity)
midimessage_set_float (set_velocity,    setVelocity)
midimessage_set_float (multiply_velocity, multiplyVelocity)

midimessage_is (program,        isProgramChange)
midimessage_get_int (program,   getProgramChangeNumber)

midimessage_is (pitch,          isPitchWheel)
midimessage_get_int (pitch,     getPitchWheelValue)

midimessage_is (aftertouch,     isAftertouch)
midimessage_get_int (aftertouch, getAfterTouchValue)

midimessage_is (pressure,       isChannelPressure)
midimessage_get_int (pressure,  getChannelPressureValue)

midimessage_is (controller,     isController)
midimessage_get_int (controller, getControllerNumber)
midimessage_get_int (controller_value, getControllerValue)
static int midimessage_is_controller_type (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushboolean (L, msg->isControllerOfType (
        lua_tointeger (L, 2)
    ));
    return 1;
}

midimessage_is (notes_off, isAllNotesOff)
midimessage_is (sound_off, isAllSoundOff)
midimessage_is (reset_controllers, isResetAllControllers)

midimessage_is (meta,               isMetaEvent)
midimessage_get_int (meta_type,     getMetaEventType)
midimessage_get_int (meta_length,   getMetaEventLength)
static int midimessage_meta_data (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getMetaEventData());
    return 1;
}

midimessage_is (track, isTrackMetaEvent)
midimessage_is (end_of_track, isEndOfTrackMetaEvent)
midimessage_is (track_name, isTrackNameEvent)

midimessage_is (text, isTextMetaEvent)
midimessage_get_string (text, getTextFromTextMetaEvent)

midimessage_is (tempo, isTempoMetaEvent)
midimessage_get_number (tempo_seconds_pqn, getTempoSecondsPerQuarterNote)
static int midimessage_tempo_ticks (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushnumber (L, msg->getTempoMetaEventTickLength (
        static_cast<short> (lua_tointeger (L, 2))));
    return 1;
}

midimessage_is (active_sense,   isActiveSense)

midimessage_is (start,          isMidiStart)
midimessage_is (stop,           isMidiStart)
midimessage_is (continue,       isMidiStart)
midimessage_is (clock,          isMidiClock)

midimessage_is (spp,            isSongPositionPointer)
midimessage_get_int (spp_beat,  getSongPositionPointerMidiBeat)

midimessage_is (quarter_frame,  isQuarterFrame)
midimessage_get_int (quarter_frame_seq, getQuarterFrameSequenceNumber)
midimessage_get_int (quarter_frame_value, getQuarterFrameValue)

midimessage_is (full_frame,     isFullFrame)
static int midimessage_full_frame_params (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    juce::MidiMessage::SmpteTimecodeType tc;
    int h,m,s,f; msg->getFullFrameParameters (h, m, s, f, tc);
    
    lua_pushinteger (L, h);
    lua_pushinteger (L, m);
    lua_pushinteger (L, s);
    lua_pushinteger (L, f);
    lua_pushinteger (L, static_cast<lua_Integer> (tc));

    return 5;
}

midimessage_is (mmc,            isMidiMachineControlMessage)
midimessage_get_int (mmc_command, getMidiMachineControlCommand)

static int midimessage_goto (lua_State* L) {
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    int h,m,s,f;
    auto res = msg->isMidiMachineControlGoto (h,m,s,f);
    lua_pushboolean (L, res);
    lua_pushinteger (L, h);
    lua_pushinteger (L, m);
    lua_pushinteger (L, s);
    lua_pushinteger (L, f);
    return 5;
}

static const luaL_Reg midimessage_methods[] = {
    { "__gc",                   midimessage_free },

    /// Methods.
    // @section methods

    /// Raw data.
    // @function MidiMessage:data
    { "data",                   midimessage_data },

    /// Message description.
    // Human readable information about the message.
    // @function MidiMessage:description
    // @treturn string
    { "description",            midimessage_description },

    /// Timestamp.
    // @function MidiMessage:time
    // @treturn number Timestamp
    { "time",                   midimessage_time },

    /// Change timestamp.
    // @function MidiMessage:settime
    // @number t Timestamp to set
    { "settime",                midimessage_set_time },

    /// Add to timestamp.
    // @function MidiMessage:addtime
    // @number dt Delta time to add
    { "addtime",                midimessage_add_time },

    /// Create copy with a new timestamp.
    // @function MidiMessage:withtime
    // @number t Timestamp to set
    // @treturn kv.MidiMessage New midi message
    { "withtime",               midimessage_with_time },

    /// Midi Channel.
    // @function MidiMessage:channel
    // @treturn int 1-16 if valid channel
    { "channel",                midimessage_channel },

    /// Is a channel message.
    // Returns true if the message has the given channel
    // @function MidiMessage:ischannel
    // @int ch Channel to check
    // @treturn bool
    { "ischannel",              midimessage_is_channel },

    /// Set the channel.
    // @function MidiMessage:setchannel
    // @int ch Channel to set (1-16)
    { "setchannel",             midimessage_set_channel },

    /// Is sysex.
    // @function MidiMessage:issysex
    // @return True if is sysex message
    { "issysex",                midimessage_is_sysex },

    /// Sysex Data.
    // @function MidiMessage:sysexdata
    // @return Raw sysex bytes
    // @return Sysex data size
    { "sysexdata",              midimessage_sysex_data },

    /// Is note on.
    // @function MidiMessage:isnoteon
    // @return True if a note on message
    { "isnoteon",               midimessage_is_note_on },

    /// Is note off.
    // @function MidiMessage:isnoteoff
    // @return True if a note on message
    { "isnoteoff",              midimessage_is_note_off },

    /// Is note on or off.
    // @function MidiMessage:isnote
    // @return True if a note on message
    { "isnote",                 midimessage_is_note },

    /// Note number.
    // @function MidiMessage:note
    // @return The note number
    { "note",                   midimessage_note },

    /// Change note number.
    // @function MidiMessage:setnote
    // @int note New note number
    { "setnote",                midimessage_set_note },

    /// Velocity.
    // @function MidiMessage:velocity
    // @treturn int Velocity (0-127)
    { "velocity",               midimessage_velocity },

    /// Float velocity.
    // @function MidiMessage:fvelocity
    // @treturn number Velocity (0.0-1.0)
    { "fvelocity",              midimessage_velocity_float },

    /// Multiply Velocity.
    // @function MidiMessage:xvelocity
    // @number m Multiplier
    { "xvelocity",              midimessage_multiply_velocity },

    /// Change velocity.
    // @function MidiMessage:setvelocity
    // @number v New velocity (0.0-1.0)
    { "setvelocity",            midimessage_set_velocity },

    /// Is program change?
    // @function MidiMessage:isprogram
    // @return True if a program change message
    { "isprogram",              midimessage_is_program },

    /// Program number.
    // @function MidiMessage:program
    // @treturn int The program number
    { "program",                midimessage_program },

    /// Is pitch message.
    // @function MidiMessage:ispitch
    // @return True if a pitch message
    { "ispitch",                midimessage_is_pitch },

    /// Pitch.
    // @function MidiMessage:pitch
    // @treturn int Pitch
    { "pitch",                  midimessage_pitch },

    /// Is after touch message.
    // @function MidiMessage:isaftertouch
    // @return True if an after touch message
    { "isaftertouch",           midimessage_is_aftertouch },

    /// After touch.
    // @function MidiMessage:aftertouch
    // @treturn int After touch value
    { "aftertouch",             midimessage_aftertouch },

    /// Is pressure message.
    // @function MidiMessage:ispressure
    // @return True if an after touch message
    { "ispressure",             midimessage_is_pressure },

    /// Pressure.
    // @function MidiMessage:pressure
    // @treturn int Presure value
    { "pressure",               midimessage_pressure },

    { "iscontroller",           midimessage_is_controller },
    { "controller",             midimessage_controller },
    { "controller_value",       midimessage_controller_value },
    { "is_controller_type",     midimessage_is_controller_type },

    { "is_notes_off",           midimessage_is_notes_off },
    { "is_sound_off",           midimessage_is_sound_off },
    { "is_reset_controllers",   midimessage_is_reset_controllers },
    
    { "ismeta",                 midimessage_is_meta },
    { "metatype",               midimessage_meta_type },
    { "metadata",               midimessage_meta_data },
    { "metalength",             midimessage_meta_length },
    
    { "istrack",                midimessage_is_track },
    { "istrackend",             midimessage_is_end_of_track },
    { "istrackname",            midimessage_is_track_name },

    { "istext",                 midimessage_is_text },
    { "text",                   midimessage_text },

    { "istempo",                midimessage_is_tempo },
    { "tempospqn",              midimessage_tempo_seconds_pqn },
    { "tempoticks",             midimessage_tempo_ticks },

    { "isactivesense",          midimessage_is_active_sense },

    { "isstart",                midimessage_is_start },
    { "isstop",                 midimessage_is_stop },
    { "iscontinue",             midimessage_is_continue },
    { "isclock",                midimessage_is_clock },
    
    { "isspp",                  midimessage_is_spp },
    { "sppbeat",                midimessage_spp_beat },

    { "is_quarter_frame",       midimessage_is_quarter_frame },
    { "quarter_frame_seq",      midimessage_quarter_frame_seq },
    { "quarter_frame_value",    midimessage_quarter_frame_value },

    { "is_full_frame",          midimessage_is_full_frame },
    { "full_frame_params",      midimessage_full_frame_params },

    { "is_mmc",                 midimessage_is_mmc },
    { "mmc_command",            midimessage_mmc_command },

    { "goto",                   midimessage_goto },

    { nullptr, nullptr }
};

LUAMOD_API
int luaopen_kv_MidiMessage (lua_State* L) {
    if (luaL_newmetatable (L, LKV_MT_MIDI_MESSAGE)) {
        lua_pushvalue (L, -1);               /* duplicate the metatable */
        lua_setfield (L, -2, "__index");     /* mt.__index = mt */
        luaL_setfuncs (L, midimessage_methods, 0);
        lua_pop (L, 1);
    }

    if (luaL_newmetatable (L, LKV_MT_MIDI_MESSAGE_TYPE)) {
        lua_pushcfunction (L, midimessage_new);   /* push audio_new function */
        lua_setfield (L, -2, "__call");     /* mt.__call = audio_new */
        lua_pop (L, 1);
    }

    lua_newtable (L);
    luaL_setmetatable (L, LKV_MT_MIDI_MESSAGE_TYPE);
    lua_pushcfunction (L, midimessage_new);
    lua_setfield (L, -2, "new");
    return 1;
}
