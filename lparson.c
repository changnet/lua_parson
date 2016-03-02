#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <math.h>
#include <limits.h>
#include <assert.h>

#include "parson.h"

#define ARRAY_KEY    "__array"
#define MAX_KEY_LEN  1024

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
        
        if ( key > *max_index ) *max_index = (int)key;

        lua_pop( L, 1 );
    }
    
    return 0;
}

static int encode_invalid_key_array( lua_State *L,int index,int pretty )
{
    return 0;
}

static int encode_array( lua_State *L,int index,int max_index,int pretty )
{
    int cur_key = 0;
    if ( max_index <= 0 ) return encode_invalid_key_array( L,index,pretty );
    
    for ( cur_key = 0;cur_key <= max_index;cur_key ++ )
    {
        
    }
    
    return 0;
}

static int do_encode_object( lua_State *L,JSON_Object *cur_object,int index )
{
    lua_pushnil( L );
    while ( lua_next( L, index ) != 0 )
    {
        /* the key and value in -2,-1,but table maybe not in -3 */
        char key[MAX_KEY_LEN];
        int ty = lua_type( L,-2 );
        switch ( ty )
        {
            case LUA_TNUMBER :
            {
                double val = lua_tonumber( L,-2 );
                snprintf( key,MAX_KEY_LEN,"%f",val );
            }break;
            case LUA_TSTRING :
            {
                const char * val = lua_tostring( L,-2 );
                /* still make a copy,in case lua string is binary */
                snprintf( key,MAX_KEY_LEN,"%s",val );
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
        
        ty = lua_type( L,-1 );
        switch ( ty )
        {
            case LUA_TNIL     :
            {
                JSON_Value *val = json_value_init_null();
                JSON_Status st = json_object_set_value( cur_object, key, val );
                if ( JSONSuccess != st )
                {
                    pmsg = "encode object nil value fail";
                    lua_pop( L,2 );
                    return -1;
                }
            }break;
            case LUA_TBOOLEAN :
            {
                JSON_Value *val = json_value_init_boolean( lua_toboolean(L,-1) );
                JSON_Status st = json_object_set_value( cur_object, key, val );
                if ( JSONSuccess != st )
                {
                    pmsg = "encode object boolean value fail";
                    lua_pop( L,2 );
                    return -1;
                }
            }break;
            case LUA_TSTRING  :
            {
                JSON_Value *val = json_value_init_string( lua_tostring(L,-1) );
                JSON_Status st = json_object_set_value( cur_object, key, val );
                if ( JSONSuccess != st )
                {
                    pmsg = "encode object string value fail";
                    lua_pop( L,2 );
                    return -1;
                }
            }break;
            case LUA_TNUMBER  :
            {
                JSON_Value *val = json_value_init_number( lua_tonumber(L,-1) );
                JSON_Status st = json_object_set_value( cur_object, key, val );
                if ( JSONSuccess != st )
                {
                    pmsg = "encode object number value fail";
                    lua_pop( L,2 );
                    return -1;
                }
            }break;
            case LUA_TTABLE   :
            {
                return 0;
            }break;
            default :
            {
                return 0;
            }
        }
        
        lua_pop( L,1 );
    }
    /*
    char *serialized_string = NULL;
    json_object_set_string(root_object, "name", "John Smith");
    json_object_set_number(root_object, "age", 25);
    json_object_dotset_string(root_object, "address.city", "Cupertino");
    json_object_dotset_value(root_object, "contact.emails", json_parse_string("[\"email@example.com\",\"email2@example.com\"]"));
    serialized_string = json_serialize_to_string_pretty(root_value);
    puts(serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);*/
    
    return 0;
}

static inline int encode_object( lua_State *L,int index,int pretty )
{
    char *str;

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object( root_value );
    
    if ( do_encode_object( L,root_object,index ) < 0 )
    {
        json_value_free( root_value );
        lua_parson_error( L );
        return 0;
    }
    
    if ( pretty )
        str = json_serialize_to_string_pretty( root_value );
    else
        str = json_serialize_to_string( root_value );
    
    lua_pushstring( L,str );
    
    json_free_serialized_string( str );
    json_value_free( root_value );

    return 1;
}

static int encode( lua_State *L )
{
    int array  = 0;
    int pretty = 0;
    int max_index  = -1;

    if ( lua_type( L,1 ) != LUA_TTABLE )
    {
        pmsg = "lua_parson encode,argument #1 table expect";
        lua_parson_error( L );
        return 0;
    }
    
    if ( is_array( L,1,&array,&max_index ) < 0 )
    {
        lua_parson_error( L );
        return 0;
    }
    
    pretty = lua_toboolean( L,2 );
    if ( array ) return encode_array( L,1,max_index,pretty );
    
    return encode_object( L,1,pretty );
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
