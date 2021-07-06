local bytes         = require ('kv.bytes')
local midi          = require ('kv.midi')

local equals        = luaunit.assertEquals

function test_midi_controller()
    for channel = 1,16 do
        equals (bytes.pack (0xb0 | (channel - 1), 55, 127),
                midi.controller (channel, 55, 127))
    end
end

function test_midi_noteon()
    for channel = 1,16 do
        equals (bytes.pack (0x90 | (channel - 1), 55, 127),
                midi.noteon (channel, 55, 127))
    end
end

function test_midi_noteoff()
    for channel = 1,16 do
        equals (bytes.pack (0x80 | (channel - 1), 55, 127),
                midi.noteoff (channel, 55, 127))
    end
end
