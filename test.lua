
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
                print("" .. v .. ",")
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


local function set_array( tb,flag )
    local _tb = tb or {}
    local mt = getmetatable( _tb ) or {}
    rawset( mt,"__array",flag )
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
        local dst_v = assert_ex(dst[k], k)
        if "table" == type(v) then
            assert_table(v, dst_v, k, ...)
        else
            assert_ex(v == dst_v, k, ...)
        end
    end

    for k, v in pairs(dst) do
        local src_v = assert_ex(src[k], k)
        if "table" == type(v) then
            assert_table(v, src_v, k, ...)
        else
            assert_ex(v == src_v, k, ...)
        end
    end
end

local test_data = {}

test_data.employees =
{
    { firstName = "Bill" , lastName = "Gates" },
    { firstName = "George" , lastName = "Bush" },
    { firstName = "Thomas" , lastName = "Carter" }
}

test_data.default =
{
    array = {[10] = "number ten"},
    sparse = {[1] = "hello", [1024] = "world"},
    float = { [1024.12345] = "float value"},
}
test_data.empty_array = set_array( {},true )
test_data.empty_object = set_array( {},false )
test_data.force_array  = set_array( { phone1 = "123456789",phone2 = "987654321" },true )
test_data.force_object = set_array( { "USA","UK","CH" },false )

local io_str = Json.encode( test_data, false, 1023 )
print( io_str )

local file_ok = Json.encode_to_file( test_data, "test.json", true, 1023 )

local file_ctx = Json.decode_from_file("test.json", true, 1023)

local io_ctx = Json.decode( io_str, false, 1023 ) -- no comment
vd( io_ctx )

assert(file_ok)

assert_table(io_ctx, file_ctx)
-- assert_table(test_data, file_ctx)
