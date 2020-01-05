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
-- @param sparse a max number to set boundary of sparse array
-- @return json string
encode(tbl, pretty, sparse)

-- decode a lua table to json string. a lua error will throw if any error occur
-- @param tbl a lua table to be decode
-- @param file a file path to output json string
-- @param pretty boolean, format json string to pretty human readable or not
-- @param sparse a max number to set boundary of sparse array
-- @return boolean
encode_to_file(tbl, file, pretty, sparse)

-- decode a json string to a lua table.a lua error will throw if any error occur
-- @param a json string
-- @param comment boolean, is the str containt comments
-- @param sparse a max number to set boundary of sparse array
-- @return a lua table
decode(str, comment, sparse)

-- decode a file content to a lua table.a lua error will throw if any error occur
-- @param a json file path
-- @param comment boolean, is the file content containt comments
-- @param sparse a max number to set boundary of sparse array
-- @return a lua table
decode_from_file(file, comment, sparse)
```

Array or Object
---------------

 * If a table has only integer key,we consider it an array.otherwise is's an object.
 * If a table's metatable has field '__array' and it's value is true,consider the table an array
 * If a table's metatable has field '__array' and it's value is false,consider the table an object
 * Empty table is an object

Note:
 * If a array has illeage index(string or float key),it's key will be replace with index 1..n when encode
 * A sparse array will be filled with 'null' when encode

Sparse Array
------------
In default, lua_parson will not check for sparse array(sparse = 0).It simply
created a large array if there a a large key in lua table, like:
```lua
local tbl = { [1024] = true }
local str = Json.encode(tbl)
-- str = [null,null,...,true] -- 1023 null
```
If sparse > 0 and any array index missing and the table has any key > sparse,
the table's key will be convert to string and encode as a object,a field
"__sparse" will be appended to this object.
```lua
local tbl = { [1024] = true }
local str = Json.encode(tbl, false, 1023)
-- str = {"1024": true, "__sparse": true}
```
At decode, if sparse > 0 and "__sparse" is found at a object, it's key will be
convert to number.
```lua
-- str = {"1024": true, "__sparse": true}
local tbl = Json.decode(str, false, 1023)
-- tbl = { [1024] = true }
```

Example
-------

See [test.lua](test.lua)

```json
{"default":{"float":{"1024.123450":"float value"},"sparse":{"1024":"world","1":"hello","__SPARSE":true},"array":[null,null,null,null,null,null,null,null,null,"number ten"]},"force_object":{"1":"USA","2":"UK","3":"CH"},"force_array":["987654321","123456789"],"empty_object":{},"employees":[{"firstName":"Bill","lastName":"Gates"},{"firstName":"George","lastName":"Bush"},{"firstName":"Thomas","lastName":"Carter"}],"empty_array":[]}
```

```lua
{
    ["empty_array"] =
    {
    }
    ["force_array"] =
    {
        [1] = "123456789",
        [2] = "987654321",
    }
    ["default"] =
    {
        ["sparse"] =
        {
            [1024] = "world",
            [1] = "hello",
        }
        ["array"] =
        {
            [10] = "number ten",
        }
        ["float"] =
        {
            ["1024.123450"] = "float value",
        }
    }
    ["force_object"] =
    {
        ["3"] = "CH",
        ["2"] = "UK",
        ["1"] = "USA",
    }
    ["empty_object"] =
    {
    }
    ["employees"] =
    {
        [1] =
        {
            ["firstName"] = "Bill",
            ["lastName"] = "Gates",
        }
        [2] =
        {
            ["firstName"] = "George",
            ["lastName"] = "Bush",
        }
        [3] =
        {
            ["firstName"] = "Thomas",
            ["lastName"] = "Carter",
        }
    }
}
```
