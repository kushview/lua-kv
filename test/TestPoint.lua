
local Point = require ('kv.Point')

function test_point()
    local pt = Point.new()
    luaunit.assertTrue (pt:isorigin())
    luaunit.assertEquals (pt.x, 0.0)
    luaunit.assertEquals (pt.y, 0.0)
end

function test_point_set_xy()
    local pt = Point.new()
    pt:setxy (100, 200)
    luaunit.assertEquals (pt.x, 100)
    luaunit.assertEquals (pt.y, 200)
end

function test_point_with_x_y()    
    local pt = Point.new()
    pt = pt:withx (100)
    pt = pt:withy (200)
    luaunit.assertEquals (pt.x, 100)
    luaunit.assertEquals (pt.y, 200)
end

function test_point_translated()    
    local pt = Point.new()
    pt = pt:translated (-100, 200)
    luaunit.assertEquals (pt.x, -100)
    luaunit.assertEquals (pt.y, 200)
end
