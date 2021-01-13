--- An audio buffer
-- @classmod kv.AudioBuffer
-- @pragma nostrip

return setmetatable ({
    --- Create a 32bit buffer
    -- @function AudioBuffer.Float
    -- @param ... Params sent to ctor
    -- @return kv.AudioBuffer
    -- @within Constructors
    Float  = require ('kv.AudioBuffer32'),

    --- Create a 64bit buffer
    -- @function AudioBuffer.Double
    -- @param ... Params sent to ctor
    -- @return kv.AudioBuffer
    -- @within Constructors
    Double = require ('kv.AudioBuffer64')
} , {
    __call = function (T, ...)
        return T.Float (...)
    end
})
