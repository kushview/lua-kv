local AudioBuffer       = require ('kv.AudioBuffer')
local round             = require ('kv.round')

TestAudioBuffer = {
    testNew = function()
        local buf = AudioBuffer.new32 (2, 128)
        print(tostring(buf))
        luaunit.assertNotEquals (buf, nil)
        luaunit.assertEquals (type (buf), 'userdata')
        luaunit.assertEquals (buf:channels(), 2)
        luaunit.assertEquals (buf:length(), 128)
        luaunit.assertFalse (buf:cleared())
        luaunit.assertTrue (buf:isfloat())
        luaunit.assertFalse (buf:isdouble())
    end,

    testGetSet = function()
        local nchans = 1
        local nframes = 4096
        local buf = AudioBuffer.new (nchans, nframes)
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
        local buf = AudioBuffer.new (2, 128)
        buf:set (1, 1, 0.0)
        luaunit.assertFalse (buf:cleared())
        buf:clear()
        luaunit.assertTrue (buf:cleared())
    end,

    tearDown = function()
        collectgarbage()
    end
}
