#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "parson.h"

static int encode( lua_State *L )
{
    lua_pushstring( L,"mde35dfafefsxee4r3" );
    return 1;
}

static int decode( lua_State *L )
{
    return 0;
}
/* ====================LIBRARY INITIALISATION FUNCTION======================= */

static const luaL_Reg lua_parson_lib[] =
{
    {"encode", encode},
    {"decode", decode},
    {NULL, NULL}
};

int luaopen_lua_parson(lua_State *L)
{
    luaL_newlib(L, lua_parson_lib);
    return 1;
}
