--- An audio buffer
-- @classmod kv.AudioBuffer
-- @pragma nostrip

return setmetatable ({
    Float  = require ('kv.AudioBuffer32'),
    Double = require ('kv.AudioBuffer64')
} , {
    __call = function (T, ...)
        return T.Float (...)
    end
})
