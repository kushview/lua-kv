local MidiBuffer    = require ('kv.MidiBuffer');
local MidiMessage   = require ('kv.MidiMessage');
local midi          = require ('kv.midi')

test_MidiBuffer = {
    testNew = function()
        local buf = MidiBuffer.new()
        luaunit.assertNotEquals (buf, nil)
        luaunit.assertEquals (buf:size(), 0)
    end,

    testClear = function()
        local buf = MidiBuffer.new()
        for f = 1,100 do buf:addmessage (MidiMessage.new(), f) end
        buf:clear()
        luaunit.assertEquals (buf:size(), 0)
    end,

    testMessages = function()
        local buf = MidiBuffer.new()
        buf:insert (midi.noteon (1, 55, 100), 1)
        buf:insert (midi.noteoff (1, 55, 0), 234)
        luaunit.assertEquals (buf:size(), 2)

        local non, noff = 0, 0
        for m, f in buf:messages() do
            if m:isnoteon() then
                non = non + 1
            elseif m:isnoteoff() then
                noff = noff + 1
            end
        end
        luaunit.assertTrue (non + noff == 2, 
            "Invalid message counts: "..non..noff)
    end,

    testReserve = function()
        local buf = MidiBuffer.new()
        buf:reserve (1024)
    end,

    testAddMessage = function()
        local buf = MidiBuffer.new()
        buf:addmessage (MidiMessage.new(), 20)
        luaunit.assertEquals (buf:size(), 1)
    end,

    testSwap = function()
        local b1 = MidiBuffer.new()
        b1:addmessage (MidiMessage.new(), 0)
        luaunit.assertEquals (b1:size(), 1)

        local b2 = MidiBuffer.new()
        b1:swap (b2)
        luaunit.assertEquals (b1:size(), 0)
        luaunit.assertEquals (b2:size(), 1)
    end,

    tearDown = function()
        collectgarbage()
    end
}
