
local Json = require "lua_parson"

local function var_dump(data, prefix)
    prefix = prefix or ""

    print("")

    local prefix_next = prefix .. "    "
    print(prefix .. "{")
    for k,v in pairs(data) do
        if "string" == type(k) then
            io.stdout:write(prefix_next .. "[\"" .. k .. "\"] = ")
        else
            io.stdout:write(prefix_next .. "[" .. k .. "] = ")
        end

        if type(v) ~= "table" then
            if "string" == type(v) then
                print("\"" .. v .. "\",")
            else
                print("" .. tostring(v) .. ",")
            end
        else
            var_dump(v, prefix_next)
        end
    end
    print(prefix .. "}")
end

--[[
    eg: local b = {aaa="aaa",bbb="bbb",ccc="ccc"}
]]
local function vd(data)
    var_dump(data)
end


local function set_array(tb, opt)
    local _tb = tb or {}
    local mt = getmetatable( _tb ) or {}
    rawset( mt,"__array_opt", opt)
    setmetatable( _tb,mt )

    return _tb
end

local function assert_ex(expr, ...)
    if not expr then
        print(...)
        assert(false)
    end

    return expr
end

local function assert_table(src, dst, ...)
    for k, v in pairs(src) do
        local dst_v = dst[k]
        if "table" == type(v) then
            assert_table(v, dst_v, k, ...)
        else
            assert_ex(v == dst_v, k, ...)
        end
    end

    for k, v in pairs(dst) do
        local src_v = src[k]
        if "table" == type(v) then
            assert_table(v, src_v, k, ...)
        else
            assert_ex(v == src_v, k, ...)
        end
    end
end


local str
local tbl
local raw_str
local raw_tbl


-- no convert, large sparse array [null, null, null, ..., 1]
raw_str = ""
for i = 1, 1023 do raw_str = raw_str .. "null," end
raw_str = "[" .. raw_str .. "1]"

raw_tbl = {
    [1024] = 1
}
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- no convert, normal array
raw_str = '[1,"a",true,2.5,false,987654321123]'
raw_tbl = {1,"a", true, 2.5, false, 987654321123}
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
print(str)
vd(tbl)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- no convert, normal object
raw_tbl = {
    ["1"] = 1,
    ["a"] = "a",
    ["true"] = true,
    ["false"] = false,
    ["987654321123"] = 987654321123,
    ["__array_opt"] = 1. -- special key test
}
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
-- key sequence are random, can not test it
-- assert(str == raw_str)

-- empty table, normal object
raw_str = '{}'
raw_tbl = {}
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- force empty array
raw_str = '[]'
raw_tbl = set_array({}, 1)
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- force empty object
raw_str = '{"__array_opt":0}'
raw_tbl = set_array({}, 0)
str = Json.encode(raw_tbl)
tbl = Json.decode(str, false, 0) -- must set the array_opt
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- force array
raw_str = '{"__array_opt":0}'
raw_tbl = set_array({ phone1 = "123456789",phone2 = "987654321" },1)
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
print(str)
vd(tbl)
assert(tbl[1] == "123456789" or tbl[2] == "123456789")
-- assert_table({}, tbl)
-- assert(str == raw_str)

-- force object
raw_str = '{"1":"AA","2":"BB","3":"CC","__array_opt":0}'
raw_tbl = set_array({"AA","BB","CC"}, 0)
str = Json.encode(raw_tbl)
tbl = Json.decode(str, false, 0) -- must set the array_opt
print(str)
vd(tbl)
assert_table(raw_tbl, tbl)
assert(str == raw_str)


-- auto check array by metatable array_opt, integer part
raw_str = ""
for i = 1, 95 do raw_str = raw_str .. "null," end
raw_str = "[" .. raw_str .. "1]"
raw_tbl = set_array({[96] = 1}, 96.6)
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- auto check array by metatable array_opt, fractional part
raw_str = "[null,null,null,null,1,1,1,1,1,1]"
raw_tbl = set_array({
    [5] = 1,
    [6] = 1,
    [7] = 1,
    [8] = 1,
    [9] = 1,
    [10] = 1
}, 8.6)
str = Json.encode(raw_tbl)
tbl = Json.decode(str)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- auto check object by metatable array_opt, integer part
raw_str = '{"96":1,"__array_opt":0}'
raw_tbl = set_array({[96] = 1}, 8.6)
str = Json.encode(raw_tbl)
tbl = Json.decode(str, false, 0)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- auto check object by metatable array_opt, fractional part
raw_str = '{"1":1,"2":1,"3":1,"4":1,"10":1,"__array_opt":0}'
raw_tbl = set_array({
    [1] = 1,
    [2] = 1,
    [3] = 1,
    [4] = 1,
    [10] = 1
}, 8.6)
str = Json.encode(raw_tbl)
tbl = Json.decode(str, false, 0)
assert_table(raw_tbl, tbl)
assert(str == raw_str)

-- auto check by array_opt
raw_str = '{"1":1,"2":1,"3":1,"4":1,"10":1,"__array_opt":0}'
raw_tbl = {
    [1] = 1,
    [2] = 1,
    [3] = 1,
    [4] = 1,
    [10] = 1
}
str = Json.encode(raw_tbl, false, 8.6)
tbl = Json.decode(str, false, 0)
assert_table(raw_tbl, tbl)
assert(str == raw_str)
