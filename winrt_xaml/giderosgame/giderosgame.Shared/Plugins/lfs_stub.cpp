#include "gideros.h"
#include "lua.h"
#include "lauxlib.h"

extern "C" {
LUALIB_API int luaopen_lfs(lua_State *L);
}

static void g_initializePlugin(lua_State *L)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    lua_pushcnfunction(L, luaopen_lfs, "luaopen_lfs");
    lua_setfield(L, -2, "lfs");

    lua_pop(L, 2);
}

static void g_deinitializePlugin(lua_State *L)
{
}
REGISTER_PLUGIN("LuaFileSystem", "1.0")
