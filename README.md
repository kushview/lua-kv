# LRT
A set of Lua libraries for use in a real time environment

#### Building
```
./waf configure
./waf build
```

#### Installing
_may need `sudo` here_
```
./waf install
```
The above will install to `$PREFIX/lib/lua/5.3`

#### Usage
```lua
local db = require ('decibels')

-- specifiy minus infinity dB (default is -100.0)
local minus_infinity = -90.0

-- convert dB to gain
local gain = db.togain (6.0, minus_infinity)

-- convert gain to dB
local vol  = db.fromgain (gain, minus_infinity)
```

#### Project Inclusion
1) Compile `decibels.c` with your code
2) Require the module by normal means.

```c
#include <lauxlib.h>

extern int luaopen_decibels (lua_State*);

void register_lua_modules (lua_State* L) {
    lua_requiref (L, "decibels", luaopen_decibels, 0);
    lua_pop (L, 1);
}
```

You can now use as described in the Usage section. This isn't the only way to do this. It is just one example.
