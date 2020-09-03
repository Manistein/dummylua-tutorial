local function print_test()
	local str = "hello world"
	print("hello world")
end

print_test()

	 local number = 0.123
	 local number2 = .456
local tbl = {}
tbl["key"] = "value" .. "value2"

function print_r(...)
	return ...
end

tbl.key

-- This is comment
tbl.sum = 100 + 200.0 - 10 * 12 / 13 % (1+2)
if tbl.sum ~= 100 then
	tbl.sum = tbl.sum << 2
elseif tbl.sum == 200 then
	tbl.sum = tbl.sum >> 2
elseif tbl.sum > 1 then 
elseif tbl.sum < 2 then
elseif tbl.sum >= 3 then 
elseif tbl.sum <= 4 then 
	tbl.sum = nil
end

tbl.true = true
tbl.false = false

local a, b = 11, 22