
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

function var_dump(data, max_level, prefix)
    if type(prefix) ~= "string" then
        prefix = ""
    end

    if type(data) ~= "table" then
        print(prefix .. tostring(data))
    else
        print(data)
        if max_level ~= 0 then
            local prefix_next = prefix .. "    "
            print(prefix .. "{")
            for k,v in pairs(data) do
                io.stdout:write(prefix_next .. k .. " = ")
                if type(v) ~= "table" or (type(max_level) == "number" and max_level <= 1) then
                    print(v)
                else
                    if max_level == nil then
                        var_dump(v, nil, prefix_next)
                    else
                        var_dump(v, max_level - 1, prefix_next)
                    end
                end
            end
            print(prefix .. "}")
        end
    end
end

--[[
    eg: local b = {aaa="aaa",bbb="bbb",ccc="ccc"}
]]
function vd(data, max_level)
    var_dump(data, max_level or 20)
end


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
print( Json.encode( tb ) )
vd( Json.decode( '[1,2,3,4,5,340282350000000000,{"a":null,"b":false,"c":"这是什么东西"}]' ) )
vd( Json.decode_from_file("test.json") )

local t = {}
t.a = t
Json.encode(t)
-- local t = Json.decode_from_file("canada.json")
-- print( "done ===========================================" )
-- 
-- local index = 0
-- for k,v in pairs( t.features[1].geometry.coordinates ) do
--     print( k,v )
--     vd( v )
--     index = index + 1
--     if index > 10 then
--         break
--     end
-- end
