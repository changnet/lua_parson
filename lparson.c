#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <math.h>
#include <limits.h>
#include <assert.h>

#include "parson.h"

#define ARRAY_KEY    "__array"
#define MAX_KEY_LEN  1024


static JSON_Value *encode_table( lua_State *L,int index );

static const char *pmsg = (char *)0;
static void lua_parson_error( lua_State *L )
{
    /* free memory */
    luaL_error( L,pmsg );
}

static int is_array( lua_State *L,int index,int *array,int *max_index )
{
    double key = 0;
    assert( array && max_index );

    /* set default value */
    *array = 0;
    *max_index = -1;
    
    if ( luaL_getmetafield( L,index,ARRAY_KEY ) )
    {
        if ( lua_toboolean( L,-1 ) ) *array = 1;

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
            *max_index = -1;
            lua_pop( L,2 ); /* pop both key and value */
            return 0;
        }
        
        key = lua_tonumber( L, -2 );
        if ( floor(key) != key || key < 1 )
        {
            *max_index = -1;
            lua_pop( L,2 );
            return 0;
        }
        
        if ( key > INT_MAX ) /* array index over INT_MAX,must be object */
        {
            *array = 0;
            *max_index = -1;
            lua_pop( L,2 );

            return 0;
        }
        
        if ( key > *max_index ) *max_index = (int)key;

        lua_pop( L, 1 );
    }
    
    if ( *max_index > 0 ) *array = 1;
    
    return 0;
}

static inline JSON_Value *encode_value( lua_State *L,int index )
{
    JSON_Value *val = (JSON_Value *)0;
    int ty = lua_type( L,index );
    switch ( ty )
    {
        case LUA_TNIL     :
        {
            val = json_value_init_null();
        }break;
        case LUA_TBOOLEAN :
        {
            val = json_value_init_boolean( lua_toboolean(L,-1) );
        }break;
        case LUA_TSTRING  :
        {
            val = json_value_init_string( lua_tostring(L,-1) );
        }break;
        case LUA_TNUMBER  :
        {
            val = json_value_init_number( lua_tonumber(L,-1) );
        }break;
        case LUA_TTABLE   :
        {
            val = encode_table( L,lua_gettop(L) );
        }break;
        default :
        {
            pmsg = "table value must be nil,string,number,table,boolean";
        }
    }
    
    return val;
}

static JSON_Value *encode_invalid_key_array( lua_State *L,int index )
{
    JSON_Status st = 0;
    JSON_Value *val = json_value_init_array();
    JSON_Array *root_array = json_value_get_array( val );
    
    int stack_top = lua_gettop( L );
    lua_pushnil( L );
    while ( lua_next( L, -2 ) != 0 )
    {
        JSON_Value *array_val = (JSON_Value*)0;
        array_val =  encode_value( L,stack_top + 2 );
        if ( !array_val )
        {
            lua_pop( L,1 );
            json_value_free( val );
            return 0;
        }
        
        st = json_array_append_value( root_array,array_val );
        if ( JSONSuccess != st )
        {
            lua_pop( L,1 );
            json_value_free( val );
            return 0;
        }

        lua_pop( L, 1 );
    }
    
    assert( val );
    return val;
}

static JSON_Value *encode_array( lua_State *L,int index,int max_index )
{
    int cur_key = 0;
    JSON_Status st = 0;
    JSON_Value *val = json_value_init_array();
    JSON_Array *root_array = json_value_get_array( val );
    
    int stack_top = lua_gettop(L);
    for ( cur_key = 1;cur_key <= max_index;cur_key ++ )
    {
        JSON_Value *array_val = (JSON_Value*)0;
        lua_rawgeti( L, -1, cur_key );
        array_val =  encode_value( L,stack_top + 1 );
        if ( !array_val )
        {
            lua_pop( L,1 );
            json_value_free( val );
            return 0;
        }
        
        st = json_array_append_value( root_array,array_val );
        if ( JSONSuccess != st )
        {
            lua_pop( L,1 );
            json_value_free( val );
            return 0;
        }

        lua_pop( L, 1 );
    }
    
    assert( val );
    return val;
}

static int do_encode_object( lua_State *L,JSON_Object *cur_object,int index )
{
    lua_pushnil( L );
    while ( lua_next( L, index ) != 0 )
    {
        JSON_Status st = 0;
        int ty = lua_type( L,-2 );
        const char *pkey = (char *)0;
        JSON_Value *obj_val = (JSON_Value *)0;

        switch ( ty )
        {
            case LUA_TNUMBER :
            {
                char key[MAX_KEY_LEN];
                double val = lua_tonumber( L,-2 );
                if ( floor(val) == val ) /* interger key */
                    snprintf( key,MAX_KEY_LEN,"%.0f",val );
                else
                    snprintf( key,MAX_KEY_LEN,"%f",val );
                pkey = key;
            }break;
            case LUA_TSTRING :
            {
                size_t len = 0;
                pkey = lua_tolstring( L,-2,&len );
                if ( len <= 0 || len > MAX_KEY_LEN )
                {
                    pmsg = "table string key too long";
                    lua_pop( L,2 );
                    return -1;
                }
            }break;
            default :
            {
                /* LUA_TNIL LUA_TBOOLEAN LUA_TTABLE LUA_TFUNCTION, LUA_TUSERDATA,
                 * LUA_TTHREAD, LUA_TLIGHTUSERDATA can't not be key
                 */
                pmsg = "table key must be a number or string";
                lua_pop( L,2 );
                return -1;
            }
        }
        assert( pkey );
        obj_val = encode_value( L,lua_gettop(L) );
        if ( !obj_val )
        {
            lua_pop( L,2 );
            return -1;
        }
        
        st = json_object_set_value( cur_object,pkey,obj_val );
        if ( JSONSuccess != st )
        {
            pmsg = "json_object_set_value fail";
            lua_pop( L,2 );
            return -1;
        }
        
        lua_pop( L,1 );
    }
    
    return 0;
}

static inline JSON_Value *encode_object( lua_State *L,int index )
{
    JSON_Value *val = json_value_init_object();
    JSON_Object *root_object = json_value_get_object( val );

    if ( do_encode_object( L,root_object,index ) < 0 )
    {
        json_value_free( val );
        return 0;
    }

    return val;
}

static JSON_Value *encode_table( lua_State *L,int index )
{
    int array  = 0;
    int max_index  = -1;
    
    if ( lua_gettop( L ) > 10240 )
    {
        pmsg = "lua parson stack overflow";
        return (JSON_Value *)0;
    }

    if ( is_array( L,index,&array,&max_index ) < 0 )
    {
        return (JSON_Value *)0;
    }
    
    if ( array )
    {
        if ( max_index > 0 )
            return encode_array( L,index,max_index );
        else
            return encode_invalid_key_array( L,index );
    }
    
    return encode_object( L,index );
}

static int encode( lua_State *L )
{
    char *str;
    int pretty = 0;
    JSON_Value *val;

    if ( lua_type( L,1 ) != LUA_TTABLE )
    {
        pmsg = "lua_parson encode,argument #1 table expect";
        lua_parson_error( L );
        return 0;
    }
    
    pretty = lua_toboolean( L,2 );
    lua_settop( L,1 );  /* remove pretty,only table in stack now */

    val = encode_table( L,1 );
    if ( !val )
    {
        lua_parson_error( L );
        return 0;
    }

    if ( pretty )
        str = json_serialize_to_string_pretty( val );
    else
        str = json_serialize_to_string( val );

    lua_pushstring( L,str );
    
    json_free_serialized_string( str );
    json_value_free( val );

    return 1;
}

static int encode_to_file( lua_State *L )
{
    int pretty = 0;
    JSON_Value *val;
    JSON_Status st = 0;
    const char *path = 0;

    if ( lua_type( L,1 ) != LUA_TTABLE )
    {
        pmsg = "lua_parson encode,argument #1 table expect";
        lua_parson_error( L );
        return 0;
    }
    
    path = luaL_checkstring( L,2 );
    pretty = lua_toboolean( L,3 );
    lua_settop( L,1 );  /* remove path,pretty,only table in stack now */

    val = encode_table( L,1 );
    if ( !val )
    {
        lua_parson_error( L );
        return 0;
    }

    if ( pretty )
        st = json_serialize_to_file_pretty( val,path );
    else
        st = json_serialize_to_file( val,path );

    json_value_free( val );

    if ( JSONSuccess != st )
        lua_pushboolean( L,0 );
    else
        lua_pushboolean( L,1 );

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
    {"encode_to_file", encode_to_file},
    {NULL, NULL}
};

int luaopen_lua_parson(lua_State *L)
{
    luaL_newlib(L, lua_parson_lib);
    return 1;
}
