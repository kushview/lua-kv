local AudioBuffer       = require ('kv.AudioBuffer')
local round             = require ('kv.round')

TestAudioBuffer = {
    testNew = function()
        local buf = AudioBuffer.Float (2, 128)
        luaunit.assertNotEquals (buf, nil)
        luaunit.assertEquals (type (buf), 'userdata')
        luaunit.assertTrue (buf:channels() == 2)
        luaunit.assertTrue (buf:length() == 128)
        luaunit.assertFalse (buf:cleared())
        luaunit.assertTrue (buf:is_float())
        luaunit.assertFalse (buf:is_double())
    end,

    testGetSet = function()
        local nchans = 1
        local nframes = 4096
        local buf = AudioBuffer (nchans, nframes)
        luaunit.assertEquals (0.0, buf:get ())
        luaunit.assertEquals (0.0, buf:get (1))
        
        for c = 1, nchans do
            for f = 1, nframes do
                local value = round.float (math.random())
                buf:set (c, f, value)
                luaunit.assertAlmostEquals (buf:get (c, f), value)
            end
        end
    end,

    testCleared = function()
        local buf = AudioBuffer (2, 128)
        buf:set (1, 1, 0.0)
        luaunit.assertFalse (buf:cleared())
        buf:clear()
        luaunit.assertTrue (buf:cleared())
    end,

    tearDown = function()
        collectgarbage()
    end
}
