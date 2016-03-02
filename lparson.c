#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <math.h>
#include <limits.h>
#include <assert.h>

#include "parson.h"

#define ARRAY_KEY    "__array"

static const char *pmsg = (char *)0;
static void lua_parson_error( lua_State *L )
{
    /* free memory */
    luaL_error( L,pmsg );
}

static int is_array( lua_State *L,int index,int *is,int *max )
{
    double key = 0;
    assert( is && max );

    /* set default value */
    *is = 0;
    *max = -1;
    
    if ( luaL_getmetafield( L,index,ARRAY_KEY ) )
    {
        if ( lua_toboolean( L,-1 ) ) *is = 1;

        lua_pop( L,1 );    /* pop metafield value */
    }
    
    /* get max array index */
    lua_pushnil( L );
    while ( lua_next( L, -2 ) != 0 )
    {
        /* stack status:table, key, value */
        /* array index must be interger and >= 1 */
        if ( lua_type( L, -2 ) != LUA_TNUMBER )
        {
            lua_pop( L,2 ); /* pop both key and value */
            return 0;
        }
        
        key = lua_tonumber( L, -2 );
        if ( floor(key) != key || key < 1 )
        {
            lua_pop( L,2 );
            return 0;
        }
        
        if ( key > INT_MAX )
        {
            lua_pop( L,2 );
            pmsg = "array key over INT_MAX";
            return -1;
        }
        
        if ( key > *max) *max = (int)key;

        lua_pop( L, 1 );
    }
    
    return 0;
}

static int encode( lua_State *L )
{
    int is  = 0;
    int max = -1;

    if ( lua_type( L,1 ) != LUA_TTABLE )
    {
        pmsg = "lua_parson encode,argument #1 table expect";
        lua_parson_error( L );
        return 0;
    }
    
    if ( is_array( L,1,&is,&max ) < 0 )
    {
        lua_parson_error( L );
        return 0;
    }
    
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
