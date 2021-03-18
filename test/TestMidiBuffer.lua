local MidiBuffer    = require ('kv.MidiBuffer');
local MidiMessage   = require ('kv.MidiMessage');
local midi          = require ('kv.midi')

test_MidiBuffer = {
    testNew = function()
        local buf = MidiBuffer ()
        luaunit.assertNotEquals (buf, nil)
        luaunit.assertEquals (buf:num_events(), 0)
    end,

    testClear = function()
        local buf = MidiBuffer()
        for f = 1,100 do buf:add_message (MidiMessage(), f) end
        buf:clear()
        luaunit.assertEquals (buf:num_events(), 0)
    end,

    testClearRange = function()
        local buf = MidiBuffer()
        buf:insert (midi.noteon (1, 55, 100), 1)
        buf:insert (midi.noteoff (1, 55, 0), 20)
        buf:clear_range (0, 10)
        luaunit.assertEquals (buf:num_events(), 1)
    end,

    testMessages = function()
        local buf = MidiBuffer()
        local on, off = 1, 234
        buf:insert (midi.noteon (1, 55, 100), on)
        buf:insert (midi.noteoff (1, 55), off)
        luaunit.assertEquals (buf:num_events(), 2)
        for m, f in buf:messages() do
            if f == on then
                luaunit.assertTrue (m:is_note_on())
            elseif f == off then
                luaunit.assertTrue (m:is_note_off())
            else
                luaunit.assertTrue (false, "Invalid frame")
            end
        end
    end,

    testReserve = function()
        local buf = MidiBuffer()
        buf:reserve (1024)
    end,

    testAddMessage = function()
        local buf = MidiBuffer()
        buf:add_message (MidiMessage(), 20)
        luaunit.assertEquals (buf:num_events(), 1)
    end,

    testSwap = function()
        local b1 = MidiBuffer()
        b1:add_message (MidiMessage(), 0)
        luaunit.assertEquals (b1:num_events(), 1)

        local b2 = MidiBuffer()
        b1:swap (b2)
        luaunit.assertEquals (b1:num_events(), 0)
        luaunit.assertEquals (b2:num_events(), 1)
    end,

    tearDown = function()
        collectgarbage()
    end
}
