# Lua KV
A set of Lua libraries for use in a real time environment.

## Building
```
./waf configure
./waf build
```

## Installing
_may need `sudo` here_
```
./waf install
```
The above will install to `$PREFIX/lib/lua/5.3`

## Usage
__Converting Decibels to Gain and Back__
```lua
local kv = require ('kv')

-- specifiy minus infinity dB (default is -100.0)
local minus_infinity = -90.0

-- convert dB to gain
local gain = kv.dbtogain (6.0, minus_infinity)

-- convert gain to dB
local vol  = kv.gaintodb (gain, minus_infinity)
```

## Project Inclusion
1) Compile the `src/*.c` files with your code.
2) Open the libraries

```cxx
#include "lua-kv.h"

void register_lua_modules (lua_State* L) {
    kv_openlibs (L, 1);
}
```

You can now use as described in the Usage section. This isn't the only way to do this. It is just one example.
