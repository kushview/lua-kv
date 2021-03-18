
local Point = require ('kv.Point')

function test_point()
    local pt = Point()
    luaunit.assertTrue (pt:is_origin())
    luaunit.assertEquals (pt.x, 0.0)
    luaunit.assertEquals (pt.y, 0.0)
end

function test_point_set_xy()
    local pt = Point()
    pt:set_xy (100, 200)
    luaunit.assertEquals (pt.x, 100)
    luaunit.assertEquals (pt.y, 200)
end

function test_point_with_x_y()    
    local pt = Point ()
    pt = pt:with_x (100)
    pt = pt:with_y (200)
    luaunit.assertEquals (pt.x, 100)
    luaunit.assertEquals (pt.y, 200)
end

function test_point_translated()    
    local pt = Point ()
    pt = pt:translated (-100, 200)
    luaunit.assertEquals (pt.x, -100)
    luaunit.assertEquals (pt.y, 200)
end
