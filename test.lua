
local Json = require "lua_parson"

--[[
table to string:
Json.encode( tb,pretty )
Json.encode_file( "finame",tb,pretty )

string to table
Json.decode( "xxxxx",comment )
Json.decode_file( "fname",comment )
]]
print( Json.encode({}) )
