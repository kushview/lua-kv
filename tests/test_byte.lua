local byte   = require ('kv.byte')
local equals = luaunit.assertEquals

function test_byte_pack()
    equals (byte.pack(), 0)
end
