local bytes  = require ('kv.bytes')
local equals = luaunit.assertEquals

function test_bytes_pack()
    equals (bytes.pack(), 0)
end
