
local object = require ('kv.object')
local Animal = object ({
    name = {
        get = function (self)
            return self._name or ""
        end,

        set = function (self, value)
            self._name = value
        end
    }
})

function Animal:init()
    self._name = "Animal"
end

function Animal:size() return "varies" end


local Dog = object (Animal)
local Dog_mt = getmetatable (Dog)
Dog_mt.__call = function ()
    return object.new (Dog)
end

function Dog:init()
    Animal.init (self)
    self.name = "Dog"
end

local BlackLab = object (Dog)
function BlackLab:init()
    Dog.init (self)
    self.name = "BlackLab"
end

function BlackLab:size() return "medium" end

local obj = object.new (Animal)
local dog = Dog()
local lab = object.new (BlackLab)

assert (obj.name == "Animal", "obj.name ~= " .. tostring(obj.name))
assert (rawget (obj, "name") == nil)
assert (obj:size() == "varies")

assert (dog.name == "Dog", "dog.name ~= " .. tostring(dog.name))
assert (dog.name ~= obj.name, "Dog name does not differ: " .. tostring (dog.name) .. " != " .. obj.name)
assert (dog:size() == obj:size())

assert (lab.name == "BlackLab")
assert (lab:size() == "medium")

local Bare = object()
function Bare:init() self.val = 100 end
function Bare:set (v) self.val = v end
function Bare:get() return self.val end
local br = object.new (Bare)
assert (br:get() == 100)
br:set (200)
assert (br:get() == 200)