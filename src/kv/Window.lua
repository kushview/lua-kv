--- A GUI Window.
-- Inherits from `kv.Widget`
-- @classmod kv.Window
-- @pragma nostrip

local object            = require ('kv.object')
local DocumentWindow    = require ('kv.DocumentWindow')
local Widget            = require ('kv.Widget')

local Window = object (Widget, {
    --- Window visibility (bool).
    -- Setting true places an OS-level window on the desktop.
    -- @class field
    -- @name Widget.desktop
    -- @within Attributes
    desktop = {
        get = function (self) return self.impl:is_on_desktop() end,
        set = function (self, value)
            if self.impl then
                if value then self.impl:add_to_desktop()
                else self.impl:remove_from_desktop() end
            end
        end
    }
})

function Window:init()
    Widget.init (self, DocumentWindow (self))
end

--- Set the widget to display.
-- @tparam el.Widget w Displayed widget
function Window:set_widget (w)
    if w then self.impl:set_widget (w) end
end

return Window
