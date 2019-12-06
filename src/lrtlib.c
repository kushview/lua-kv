
#include "luainc.h"
#include "lrt/lrt.h"

extern int luaopen_audio (lua_State*);
extern int luaopen_midi (lua_State*);

static const luaL_Reg lrtmods[] = {
    { "audio", luaopen_audio },
    { "midi",  luaopen_midi },
    { NULL, NULL }
};

void lrt_openlibs (lua_State* L, int glb) {
    const luaL_Reg* mod = NULL;
    for (mod = lrtmods; mod->func; mod++) {
        luaL_requiref (L, mod->name, mod->func, glb);
        lua_pop (L, 1);  /* remove lib */
    }
}
