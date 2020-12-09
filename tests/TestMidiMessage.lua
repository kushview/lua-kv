local MidiMessage = require ('kv.MidiMessage')
local midi = require ('kv.midi')
local byte = require ('kv.byte')

test_MidiMessage = {
    testNew = function()
        local msg = MidiMessage()
        luaunit.assertNotEquals (msg, nil)
    end,

    testNote = function()
        local channel = 2
        local on = MidiMessage (midi.noteon (channel, 64, 100))
        luaunit.assertTrue (on:is_note_on())
        local off = MidiMessage (byte.pack (0x80 | (channel - 1), 55, 127))
        luaunit.assertTrue (off:is_note_off())
        luaunit.assertTrue (off:is_note() and on:is_note())
    end,

    tearDown = function()
        collectgarbage()
    end
}
