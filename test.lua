
local Json = require "lua_parson"

--[[
table to string:
Json.encode( tb,pretty )
Json.encode_file( "finame",tb,pretty )

string to table
Json.decode( "xxxxx",comment )
Json.decode_file( "fname",comment )


encode test
1.double as key(有、没有小数点)

local tb = 
{
    obj = 
    {
        789546213.2356 = "abcedfd",.
        4353245436 = "efght",
    }
}
]]

local function make_array( tb )
    local _tb = tb or {}
    local mt = getmetatable( _tb ) or {}
    rawset( mt,"__array",true )
    setmetatable( _tb,mt )
    
    return _tb
end

local tb = 
{
    ["abc"] = 999,
    ["efg"] = "kdfajsfjieofjadaf;dsfdsakfjasdlkfja;lsf", 
    [9999]  = 
    {
        ["zzz"] = 987890,
    },
    ["ay"] = { 9,8,5,"dfeadfea" },
    ["sparse"] = 
    {
        [10] = 8787,
    },
    ["invalid_key"] = 
    {
        [88987.00998] = 98766,
        ["dfeadfewa"] = "dfkekjdudkejdn",
    }
}

make_array( tb.invalid_key )
print( Json.encode_to_file( tb,"test.json",true ) )
