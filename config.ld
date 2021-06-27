project     = 'Lua KV'
title       = 'Lua KV'
description = 'KV Lua Modules and Classes'
file = {
    'src/kv',
    'kv/lua/rectangle.hpp',
    'kv/lua/widget.hpp',
    exclude = {
        'src/lua-kv.h',
        'src/kv/vector.c',
        'src/kv/File.cpp'
    }
}
dir         = 'build/doc/lua'
output      = 'index'
merge       = true
all         = false
--package     = 'kv'
add_language_extension ('.hpp', 'c')

---[[
custom_tags = {
    {
        "property",
        title = "Properties",
        hidden = false,
        format = function (data)
            return data .. ""
        end
    }
}

custom_display_name_handler = function (item, default_handler)
    if item.tags.kind then
        return item.name
    end
    return default_handler (item)
end
--]]
