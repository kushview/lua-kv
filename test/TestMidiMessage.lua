local MidiMessage = require ('kv.MidiMessage')
local midi = require ('kv.midi')
local bytes = require ('kv.bytes')

test_MidiMessage = {
    testNew = function()
        local msg = MidiMessage.new()
        luaunit.assertNotEquals (msg, nil)
    end,

    testNote = function()
        local channel = 2
        local on = MidiMessage.new (midi.noteon (channel, 64, 100))
        luaunit.assertTrue (on:isnoteon())
        local off = MidiMessage.new (bytes.pack (0x80 | (channel - 1), 55, 127))
        luaunit.assertTrue (off:isnoteoff())
        luaunit.assertTrue (off:isnote() and on:isnote())
    end,

    tearDown = function()
        collectgarbage()
    end
}
