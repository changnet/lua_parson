lua_parson
==========

[![Build Status](https://travis-ci.org/changnet/lua_parson.svg?branch=master)](https://travis-ci.org/changnet/lua_parson)

A lua json encode/decode c module base on parson.
See more about parson at https://github.com/kgabis/parson

Installation
------------

 * Make sure lua develop environment already installed
 * git clone https://github.com/changnet/lua_parson.git
 * cd lua_parson
 * git submodule update --init --recursive
 * make
 * make test
 * Copy lua_parson.so to your lua project's c module directory or link against liblua_parsion.a

Api
-----

```lua
-- encode a lua table to json string. a lua error will throw if any error occur
-- @param tbl a lua table to be decode
-- @param pretty boolean, format json string to pretty human readable or not
-- @param array_opt number, option to set the table as array or object
-- @return json string
encode(tbl, pretty, array_opt)

-- encode a lua table to json string. a lua error will throw if any error occur
-- @param tbl a lua table to be decode
-- @param file a file path that json string will be written in
-- @param pretty boolean, format json string to pretty human readable or not
-- @param array_opt number, option to set the table as array or object
-- @return boolean
encode_to_file(tbl, file, pretty, array_opt)

-- decode a json string to a lua table.a lua error will throw if any error occur
-- @param a json string
-- @param comment boolean, is the str containt comments
-- @param array_opt number, enable integer key convertion if set
-- @return a lua table
decode(str, comment, array_opt)

-- decode a file content to a lua table.a lua error will throw if any error occur
-- @param a json file path
-- @param comment boolean, is the file content containt comments
-- @param array_opt number, enable integer key convertion if set
-- @return a lua table
decode_from_file(file, comment, array_opt)
```

Array or Object
---------------

* No array_opt(no array_opt pass to encode、decode and no `__array_opt` in metatable)
    * Empty table is an object
    * If a table has only integer keys, consider it an array

* Detect by array_opt
    * If a table's metatable has field `__array_opt = 1` consider it an array
    * If a table's metatable has field `__array_opt = 0` consider it an object
    * `modf(array_opt)` the integral part means the table is array if table max
    key <= this value. The fractional part means table is array when
    (table size)/(max key) >= this value.
    ```lua
    -- array_opt = 8.6

    -- max key 7 <= 8, it encode as array
    local tbl = {[7] = 1}

    -- table size = 6, max key = 10, 6/10 >= 0.6, it encode as array
    local tbl = {1,2,3,4,5, [10] = 10}

    -- max key 1024 > 8 and table size = 1, 1/1024 < 0.6, it encode as object
    local tbl = {[1024] = 1}
    ```


Note:

If a table is converted to an object, a field `__array_opt` will be append to the
object. When decode, if `__array_opt` is found in the object, it's number key will
be converted to number. e.g.
```lua
local tbl = {
    [1024] = "abc",
}
```
to json
```json
{"1024": "abc", "__array_opt": 0}
```
to lua again
```lua
local tbl = {
    [1024] = "abc",
}

-- not
-- local tbl = {
--     ["1024"] = "abc",
-- }
```
This feature allows convertion between json and lua table.

Example
-------

See [test.lua](test.lua)

* normal
```json
[1,"a",true,2.5,false,987654321123]
```
```lua
{
    [1] = 1,
    [2] = "a",
    [3] = true,
    [4] = 2.5,
    [5] = false,
    [6] = 987654321123,
}
```

* table to json(array) to table
```lua
{
    phone1 = "123456789",
    phone2 = "987654321"
}
```
```json
["123456789","987654321"]
```
```lua
{
    [1] = "123456789",
    [2] = "987654321",
}
```

* table to json(object) to table
```lua
{"AA","BB","CC"}
```
```json
{"1":"AA","2":"BB","3":"CC","__array_opt":0}
```
```lua
{
    [1] = "AA",
    [2] = "BB",
    [3] = "CC",
}
```
