--- A Generic object library
-- @module kv.object
-- @usage
-- local Animal = object()
local M = {}

local function make_proxy (T, impl, expose)
    expose = expose or false
    impl = impl or {}
    setmetatable (impl, T)
    local atts = getmetatable(T).__atts or false
    local fallback = expose and impl or T

    return setmetatable ({}, {
        __impl  = impl or {},
        __index = atts and function (obj, k)
            local p = atts [k]
            if p and p.get then
                return p.get (obj)
            else
                return fallback[k]
            end
        end
        or fallback,

        __newindex = atts and function (obj, k, v)
            local p = atts [k]
            if p and p.set then
                p.set (obj, v)
            else
                rawset (obj, k, v)
            end
        end
        or fallback
    })
end

local function instantiate (T, ...)
    local obj = make_proxy (T, {})

    if 'function' == type (rawget (T, 'init')) then
        T.init (obj, ...)
    end

    return obj
end

local function define_type (...)
    local nargs = select ("#", ...)
    local B = {}
    local D = {}
    local atts = {}

    if nargs == 0 then
        atts = {}

    elseif nargs == 1 and 'table' == type (select (1, ...)) then
        local param = select (1, ...)
        if getmetatable(param) then
            B = param
            if getmetatable(B).__atts then
                for k,v in pairs (getmetatable(B).__atts) do
                    atts[k] = v
                end
            end
        else
            atts = param
        end

    elseif nargs > 1 then
        B = select (1, ...)
        if getmetatable(B).__atts then
            for k,v in pairs (getmetatable(B).__atts) do
                atts[k] = v
            end
        end
        for k,v in pairs (select (2, ...)) do
            atts[k] = v
        end
    end

    for k, v in pairs (B) do
        D[k] = v 
    end

    return setmetatable (D, {
        __atts  = atts,
        __base  = B
    })
end

    --- Define a new object type.
-- Call when you need to define a custom object. You can also invoke the module
-- directly, which is an alias to `define`.
-- @class function
-- @name object.define
-- @param base
-- @treturn table The new object type table
-- @usage
-- local object = require ('kv.object')
-- -- Verbose method
-- local Animal = object.define()
-- -- Calling the module
-- local Cat = object (Animal)
M.define = define_type

--- Create a new instance `T`.
-- @tparam table T The object type to create
-- @tparam any ... Arguments passed to `Object:init`
-- @treturn table The newly created object `T`
function M.new (T, ...)
    return instantiate (T, ...)
end

setmetatable (M, {
    __call = function (O, ...)
        return define_type (...)
    end
})

return M
