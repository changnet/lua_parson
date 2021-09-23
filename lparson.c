#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h> // strtoll
#include <string.h> // strcmp

#include "lparson.h"
#include "parson.h"

#define MAX_KEY_LEN    256
#define MAX_STACK_DEEP 1024
#define ARRAY_OPT     "__opt"

/* ========================== lua 5.1 ======================================= */
#ifndef LUA_MAXINTEGER
    #define LUA_MAXINTEGER (2147483647)
#endif

#ifndef LUA_MININTEGER
    #define LUA_MININTEGER (-2147483647 - 1)
#endif

/* this function was copy from src of lua5.3.1 */
void luaL_setfuncs_ex(lua_State *L, const luaL_Reg *l, int nup)
{
    luaL_checkstack(L, nup, "too many upvalues");
    for (; l->name != NULL; l++)
    { /* fill the table with given functions */
        int i;
        for (i = 0; i < nup; i++) /* copy upvalues to the top */
            lua_pushvalue(L, -nup);
        lua_pushcclosure(L, l->func, nup); /* closure with those upvalues */
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_pop(L, nup); /* remove upvalues */
}

#ifndef luaL_newlibtable
    #define luaL_newlibtable(L, l) \
        lua_createtable(L, 0, sizeof(l) / sizeof((l)[0]) - 1)
#endif

#ifndef luaL_newlib
    #define luaL_newlib(L, l) \
        (luaL_newlibtable(L, l), luaL_setfuncs_ex(L, l, 0))
#endif

/* ========================== lua 5.1 ======================================= */

typedef const char *lpe_t; // lua_parson_error_t
struct lpo                 // lua parson option
{
    double opt;
    lpe_t error;
};

static JSON_Value *encode_lua_value(lua_State *L, int index, struct lpo *option);
static int decode_parson_value(lua_State *L, const JSON_Value *js_val,
                               struct lpo *option);

static inline void lua_parson_error(lua_State *L, struct lpo *option)
{
    luaL_error(L, option->error);
}

/**
 * check a lua table is object or array
 * @return object = 0, max_index = 1 if using opt
 *         array = 1, max_index is max array index
 */
static int check_type(lua_State *L, int index, lua_Integer *max_index, double opt)
{
    lua_Integer key   = 0;
    int key_count     = 0;
    double index_opt  = 0.;
    double percent_opt = 0.;
    lua_Integer max_key = 0;
#if LUA_VERSION_NUM < 503
    double num_key    = 0.;
#endif

    if (luaL_getmetafield(L, index, ARRAY_OPT))
    {
        if (LUA_TNUMBER == lua_type(L, -1)) opt = lua_tonumber(L, -1);

        lua_pop(L, 1); /* pop metafield value */
    }

    // encode as object, using opt
    if (0. == opt)
    {
        *max_index = 1;
        return 0;
    }

    if (opt > 1) percent_opt = modf(opt, &index_opt);

    // try to find out the max key
    lua_pushnil(L);
    while (lua_next(L, index) != 0)
    {
        /* stack status:table, key, value */
#if LUA_VERSION_NUM >= 503 // lua 5.3 has int64
        if (!lua_isinteger(L, -2))
        {
            goto NO_MAX_KEY;
        }
        key = lua_tointeger(L, -2);
        if (key < 1)
        {
            goto NO_MAX_KEY;
        }
#else
        if (lua_type(L, -2) != LUA_TNUMBER)
        {
            goto NO_MAX_KEY;
        }
        num_key = lua_tonumber(L, -2);
        key = floor(num_key);
        /* array index must be interger and >= 1 */
        if (key < 1 || key != num_key)
        {
            goto NO_MAX_KEY;
        }
#endif
        key_count++;
        if (key > max_key) max_key = key;

        lua_pop(L, 1);
    }

    if (opt != 1.0)
    {
        // empty table encode as object
        if (0 == max_key) return 0;

        // max_key larger than opt integer part and then item count fail
        // to fill the percent of opt fractional part, encode as object
        if (opt > 0. && max_key > index_opt
            && ((double)key_count) / max_key < percent_opt)
        {
            *max_index = 1;
            return 0;
        }
    }

    *max_index = max_key;
    return 1;

NO_MAX_KEY:
    lua_pop(L, 2);
    if (1. == opt)
    {
        // force encode as array, no max key found
        *max_index = -1;
        return 1;
    }
    else
    {
        // none integer key found, encode as object
        if (opt >= 0) *max_index = 1;
        return 0;
    }
}

static inline JSON_Value *encode_value(lua_State *L, int index, struct lpo *option)
{
    JSON_Value *val = (JSON_Value *)0;
    int ty          = lua_type(L, index);
    switch (ty)
    {
    case LUA_TNIL:
    {
        val = json_value_init_null();
    }
    break;
    case LUA_TBOOLEAN:
    {
        val = json_value_init_boolean(lua_toboolean(L, -1));
    }
    break;
    case LUA_TSTRING:
    {
        val = json_value_init_string(lua_tostring(L, -1));
    }
    break;
    case LUA_TNUMBER:
    {
        val = json_value_init_number(lua_tonumber(L, -1));
    }
    break;
    case LUA_TTABLE:
    {
        val = encode_lua_value(L, lua_gettop(L), option);
    }
    break;
    default:
    {
        option->error = "table value must be nil,string,number,table,boolean";
    }
    }

    return val;
}

static JSON_Value *encode_invalid_key_array(lua_State *L, int index,
                                            struct lpo *option)
{
    JSON_Status st         = 0;
    JSON_Value *val        = json_value_init_array();
    JSON_Array *root_array = json_value_get_array(val);

    int stack_top = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, index) != 0)
    {
        JSON_Value *array_val = (JSON_Value *)0;
        array_val             = encode_value(L, stack_top + 2, option);
        if (!array_val)
        {
            lua_pop(L, 2);
            json_value_free(val);
            return 0;
        }

        st = json_array_append_value(root_array, array_val);
        if (JSONSuccess != st)
        {
            lua_pop(L, 2);
            json_value_free(val);
            return 0;
        }

        lua_pop(L, 1);
    }

    assert(val);
    return val;
}

static JSON_Value *encode_array(lua_State *L, int index, int max_index,
                                struct lpo *option)
{
    int cur_key            = 0;
    JSON_Status st         = 0;
    JSON_Value *val        = json_value_init_array();
    JSON_Array *root_array = json_value_get_array(val);

    int stack_top = lua_gettop(L);
    for (cur_key = 1; cur_key <= max_index; cur_key++)
    {
        JSON_Value *array_val = (JSON_Value *)0;
        lua_rawgeti(L, -1, cur_key);
        array_val = encode_value(L, stack_top + 1, option);
        if (!array_val)
        {
            lua_pop(L, 1);
            json_value_free(val);
            return 0;
        }

        st = json_array_append_value(root_array, array_val);
        if (JSONSuccess != st)
        {
            lua_pop(L, 1);
            json_value_free(val);
            return 0;
        }

        lua_pop(L, 1);
    }

    assert(val);
    return val;
}

static inline JSON_Value *encode_object(lua_State *L, int index, int use_cvt,
                                        struct lpo *option)
{
    JSON_Value *root_val     = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_val);

    int converted = 0;

    lua_pushnil(L);
    while (lua_next(L, index) != 0)
    {
        JSON_Status st = 0;
        char key[MAX_KEY_LEN];
        int ty              = lua_type(L, -2);
        const char *pkey    = (char *)0;
        JSON_Value *obj_val = (JSON_Value *)0;

        switch (ty)
        {
        case LUA_TNUMBER:
        {
            converted = 1;
#if LUA_VERSION_NUM >= 503 // lua 5.3 has int64
            if (lua_isinteger(L, -2))
                snprintf(key, MAX_KEY_LEN, "%lld", lua_tointeger(L, -2));
            else
                snprintf(key, MAX_KEY_LEN, "%f", lua_tonumber(L, -2));
#else
            double val = lua_tonumber(L, -2);
            if (floor(val) == val) /* interger key */
                snprintf(key, MAX_KEY_LEN, "%.0f", val);
            else
                snprintf(key, MAX_KEY_LEN, "%f", val);
#endif
            pkey = key;
        }
        break;
        case LUA_TSTRING:
        {
            size_t len = 0;
            pkey       = lua_tolstring(L, -2, &len);
            if (len <= 0 || len > MAX_KEY_LEN)
            {
                option->error = "table string key too long";
                lua_pop(L, 2);
                json_value_free(root_val);
                return 0;
            }
        }
        break;
        default:
        {
            /* LUA_TNIL LUA_TBOOLEAN LUA_TTABLE LUA_TFUNCTION, LUA_TUSERDATA,
             * LUA_TTHREAD, LUA_TLIGHTUSERDATA can't not be key
             */
            option->error = "table key must be a number or string";
            lua_pop(L, 2);
            json_value_free(root_val);
            return 0;
        }
        }

        assert(pkey);
        obj_val = encode_value(L, lua_gettop(L), option);
        if (!obj_val)
        {
            lua_pop(L, 2);
            json_value_free(root_val);
            return 0;
        }

        st = json_object_set_value(root_object, pkey, obj_val);
        if (JSONSuccess != st)
        {
            option->error = "json_object_set_value fail";
            lua_pop(L, 2);
            json_value_free(root_val);
            return 0;
        }

        lua_pop(L, 1);
    }

    // append `__array_opt` to object if using opt
    if (converted && use_cvt >= 0)
    {
        json_object_set_value(root_object,
            ARRAY_OPT, json_value_init_number(0));
    }

    return root_val;
}

static JSON_Value *encode_lua_value(lua_State *L, int index, struct lpo *option)
{
    lua_Integer max_index = -1;

    if (lua_gettop(L) > MAX_STACK_DEEP)
    {
        option->error = "lua parson encode too deep";
        return (JSON_Value *)0;
    }

    if (!lua_checkstack(L, 2))
    {
        option->error = "lua parson stack overflow";
        return (JSON_Value *)0;
    }

    int type = check_type(L, index, &max_index, option->opt);
    if (0 == type)
    {
        return encode_object(L, index, (int)max_index, option);
    }

    return max_index > 0 ? encode_array(L, index, (int)max_index, option)
                         : encode_invalid_key_array(L, index, option);
}

/**
 * encode a lua table to json string. a lua error will throw if any error occur
 * @param tbl a lua table to be encode
 * @param pretty boolean, format json string to pretty human readable or not
 * @param opt number, option to set the table as array or object
 * @return json string
 */
static int encode(lua_State *L)
{
    int pretty = 0;
    char *str;
    JSON_Value *val;

    struct lpo option;
    option.error = (lpe_t)0;

    if (lua_type(L, 1) != LUA_TTABLE)
    {
        option.error = "lua_parson encode,argument #1 table expect";
        lua_parson_error(L, &option);
        return 0;
    }

    pretty     = lua_toboolean(L, 2);
    option.opt = luaL_optnumber(L, 3, -1);
    lua_settop(L, 1); /* remove other parameters,only table in stack now */

    val = encode_lua_value(L, 1, &option);
    if (!val)
    {
        lua_parson_error(L, &option);
        return 0;
    }

    str = pretty ? json_serialize_to_string_pretty(val)
                 : json_serialize_to_string(val);

    lua_pushstring(L, str);

    json_free_serialized_string(str);
    json_value_free(val);

    return 1;
}

/**
 * encode a lua table to file. a lua error will throw if any error occur
 * @param tbl a lua table to be encode
 * @param file a file path that json string will be written in
 * @param pretty boolean, format json string to pretty human readable or not
 * @param opt number, option to set the table as array or object
 * @return boolean
 */
static int encode_to_file(lua_State *L)
{
    int pretty = 0;
    JSON_Value *val;
    JSON_Status st   = 0;
    const char *path = 0;

    struct lpo option;
    option.error = (lpe_t)0;

    if (lua_type(L, 1) != LUA_TTABLE)
    {
        option.error = "lua_parson encode,argument #1 table expect";
        lua_parson_error(L, &option);
        return 0;
    }

    path       = luaL_checkstring(L, 2);
    pretty     = lua_toboolean(L, 3);
    option.opt = luaL_optnumber(L, 4, -1);
    lua_settop(L, 1); /* remove path,pretty,only table in stack now */

    val = encode_lua_value(L, 1, &option);
    if (!val)
    {
        lua_parson_error(L, &option);
        return 0;
    }

    st = pretty ? json_serialize_to_file_pretty(val, path)
                : json_serialize_to_file(val, path);

    json_value_free(val);

    lua_pushboolean(L, JSONSuccess == st ? 1 : 0);

    return 1;
}

static int decode_parson_array(lua_State *L, const JSON_Value *js_val,
                               struct lpo *option)
{
    size_t index      = 0;
    JSON_Array *array = json_value_get_array(js_val);
    size_t count      = json_array_get_count(array);

    lua_createtable(L, (int)count, 0);
    for (index = 0; index < count; index++)
    {
        JSON_Value *temp_value = json_array_get_value(array, index);
        int stack_num          = decode_parson_value(L, temp_value, option);
        if (stack_num < 0)
        {
            lua_pop(L, 1);  /* error, pop the table */
            return -1;
        }
        else if (stack_num > 0)
        {
            assert(1 == stack_num);
            /* lua table index start from 1 */
            lua_rawseti(L, -2, index + 1);
        }
        /* stack_num == 0,value is nil */
    }

    return 1;
}

static int decode_parson_object(lua_State *L, const JSON_Value *js_val,
                               struct lpo *option)
{
    int is_convert      = 0;
    size_t index        = 0;
    JSON_Object *object = json_value_get_object(js_val);
    size_t count        = json_object_get_count(object);

    if (option->opt != 1 && option->opt >= 0)
    {
        if (json_object_get_value(object, ARRAY_OPT))
        {
            count--;
            is_convert = 1;
        }
    }

    lua_createtable(L, 0, (int)count);
    for (index = 0; index < count; index++)
    {
        int stack_num   = -1;
        const char *key = json_object_get_name(object, index);
        if (is_convert && 0 == strcmp(key, ARRAY_OPT))
        {
            continue;
        }
        JSON_Value *temp_value = json_object_get_value(object, key);

        if (is_convert)
        {
            char *end_ptr = NULL;
            long long int ikey = strtoll(key, &end_ptr, 0);
            // *end_ptr != '\0' if convert error or key is float
            // if can not convert to integer, push the string key
            if (*end_ptr != '\0')
            {
                double d = strtod(key, &end_ptr);
                if (*end_ptr != '\0')
                    lua_pushstring(L, key);
                else
                    lua_pushnumber(L, d);
            }
            else
            {
#if LUA_VERSION_NUM >= 503 // int64 support
                lua_pushinteger(L, ikey);
#else
                lua_pushnumber(L, ikey);
#endif
            }
        }
        else
        {
            lua_pushstring(L, key);
        }

        stack_num = decode_parson_value(L, temp_value, option);
        if (stack_num < 0)
        {
            lua_pop(L, 2); /* error, pop the key and table */
            return -1;
        }
        else if (stack_num > 0)
        {
            assert(1 == stack_num);
            lua_rawset(L, -3);
        }
        else
        {
            lua_pop(L, 1); /* null, pop the key */
        }
    }

    return 1;
}

static int decode_parson_value(lua_State *L, const JSON_Value *js_val,
                               struct lpo *option)
{
    int push_num  = 1;
    int stack_top = lua_gettop(L);
    if (stack_top > MAX_STACK_DEEP)
    {
        option->error = "lua parson decode too deep";
        return -1;
    }

    if (!lua_checkstack(L, 3))
    {
        option->error = "lua parson stack overflow";
        return -1;
    }

    switch (json_value_get_type(js_val))
    {
    case JSONArray:
    {
        if (decode_parson_array(L, js_val, option) < 0) return -1;
    }
    break;
    case JSONObject:
    {
        if (decode_parson_object(L, js_val, option) < 0) return -1;
    }
    break;
    case JSONString:
    {
        const char *val = json_value_get_string(js_val);
        lua_pushstring(L, val);
    }
    break;
    case JSONBoolean:
    {
        int val = json_value_get_boolean(js_val);
        lua_pushboolean(L, val);
    }
    break;
    case JSONNumber:
    {
        double val = json_value_get_number(js_val);
        /* if using lua5.1,interger is int32 */
        if (floor(val) == val && (val <= LUA_MAXINTEGER && val >= LUA_MININTEGER))
            lua_pushinteger(L, (lua_Integer)val);
        else
            lua_pushnumber(L, val);
    }
    break;
    case JSONNull:
    {
        push_num = 0;
    }
    break;
    case JSONError:
    {
        option->error = "parson get error type";
        return -1;
    }
    break;
    default:
    {
        option->error = "unhandle json value type";
        return -1;
    }
    }

    assert(0 == push_num || 1 == push_num);
    return push_num;
}

/**
 * decode a json string to a lua table.a lua error will throw if any error occur
 * @param a json string
 * @param comment boolean, is the str containt comments
 * @param opt number, enable integer key convertion if set
 * @return a lua table
 */
static int decode(lua_State *L)
{
    int return_code = 0;
    JSON_Value *val = (JSON_Value *)0;
    const char *str = luaL_checkstring(L, 1);
    int comment     = lua_toboolean(L, 2);

    struct lpo option;
    option.error = (lpe_t)0;
    option.opt   = luaL_optnumber(L, 3, -1);

    assert(str);

    val = comment ? json_parse_string_with_comments(str) : json_parse_string(str);

    if (!val)
    {
        option.error = "invalid json string";
        lua_parson_error(L, &option);
        return 0;
    }

    return_code = decode_parson_value(L, val, &option);
    json_value_free(val);

    if (return_code < 0)
    {
        lua_parson_error(L, &option);
        return 0;
    }

    /* decode string "null",0 == return_code */
    assert(0 == return_code || 1 == return_code);
    return return_code;
}

/**
 * decode a file content to a lua table.a lua error will throw if any error
 * occur
 * @param a json file path
 * @param comment boolean, is the file content containt comments
 * @param opt number, enable integer key convertion if set
 * @return a lua table
 */
static int decode_from_file(lua_State *L)
{
    int return_code  = 0;
    JSON_Value *val  = (JSON_Value *)0;
    const char *path = luaL_checkstring(L, 1);
    int comment      = lua_toboolean(L, 2);

    struct lpo option;
    option.error = (lpe_t)0;
    option.opt   = luaL_optnumber(L, 3, -1);

    assert(path);

    val = comment ? json_parse_file_with_comments(path) : json_parse_file(path);

    if (!val)
    {
        option.error = "invalid json string";
        lua_parson_error(L, &option);
        return 0;
    }

    return_code = decode_parson_value(L, val, &option);
    json_value_free(val);

    if (return_code < 0)
    {
        lua_parson_error(L, &option);
        return 0;
    }

    /* decode string "null",0 == return_code */
    assert(0 == return_code || 1 == return_code);
    return return_code;
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */

static const luaL_Reg lua_parson_lib[] = {{"encode", encode},
                                          {"decode", decode},
                                          {"encode_to_file", encode_to_file},
                                          {"decode_from_file", decode_from_file},
                                          {NULL, NULL}};

int luaopen_lua_parson(lua_State *L)
{
    luaL_checkversion(L);
    luaL_newlib(L, lua_parson_lib);
    return 1;
}
