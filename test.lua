
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

local tb = 
{
    ["abc"] = 999,
    ["efg"] = "kdfajsfjieofjadaf;dsfdsakfjasdlkfja;lsf", 
    [9999]  = 
    {
        ["zzz"] = 987890,
    }
}
print( Json.encode( tb,true ) )
