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
local audio = require ('audio')

-- specifiy minus infinity dB (default is -100.0)
local minus_infinity = -90.0

-- convert dB to gain
local gain = audio.dbtogain (6.0, minus_infinity)

-- convert gain to dB
local vol  = audio.gaintodb (gain, minus_infinity)
```

#### Project Inclusion
1) Compile `src/*.c` with your code
2) Require the module by normal means.

```c
#include <lauxlib.h>

extern int luaopen_audio (lua_State*);

void register_lua_modules (lua_State* L) {
    lua_requiref (L, "audio", luaopen_audio, 0);
    lua_pop (L, 1);
}
```

You can now use as described in the Usage section. This isn't the only way to do this. It is just one example.
