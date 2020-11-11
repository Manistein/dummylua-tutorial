-- test local
print("-- test local")
local var1, var2
print(var1, var2)

-- test do end
print("-- test do end")
do
	print("do beigin")

	local var, var2 = 1 + 2 * 3

	print(var, var2)
	print("do end")
end
print(var)

-- test ifstat
print("-- test ifstat")
local a, b, c = 10, 20, 3
local d
if a >= b then
	print("a>=b")
elseif a <= c then 
	print("a<=c")
	d = 1 * 2 + 3
else
	print("else-part")
end

print("2")
print(c)
print(d)

-- test while
print("-- test while")
local i = 0
local j = 0
local k
while i < 100 do 
	i = i + 1
	if i > 50 then 
		while j < 10 do 
			j = j + 1
			break
		end 
		break
	end 
	k = i + j
end 
print(k)
print(i)
print(j)

-- test for
print("-- test fornum")
do 
	local v = 0
	for i = 1, 10, 0.1 do 
		v = v + 1
		if v == 10 then
			break
		end
	end 
	print(v)
end 

local v = 10
print(v)
for i = 10, 1, -1 do 
	v = v - 1
end 
print(v)

print("-- test forlist")
local v1 = 0
local t = {3, 9, 7, 5, ["k1"] = "v1", k2 = "v2" }
for k, v in pairs(t) do 
	print(k, v)
	for i = 1, 15 do 
		v1 = v1 + 1
		if v1 == 12 then 
			break
		end 
	end 
	print(v1)
end 
print(v1)

-- test repeat
print("-- test repeat")
local v2 = 0
print(v2)
repeat
 v2 = v2 + 1
 if v2 == 10 then 
 	break	
 end 
until v2 > 100
print(v2)

-- test function
print("-- test function")
function foo(year, month, day)
	local ret = year * month + day
	print(ret, year, month, day)
end
foo(1, 2, 3)

local t = {}
function t.foo(arg1, arg2)
	print("hello", arg1, arg2)
end

function t:bar(arg1, arg2)
	print("t:bar", arg1, arg2)
	self.ret = arg1 - arg2
	print(self.ret)
end

t.foo(2020, 1024)
t:bar(2020, 2021)
t:bar(2021, 1024)

local function bar(arg1, arg2, arg3)
	local ret = arg1 * arg2 + arg3
	print(ret, arg1, arg2, arg3)
end 
bar(1, 2, 3)

g = 4
h = 5
function test_return(a, b, c)
	local d, e, f = 1, 2, 3
	f = g + h
	-- print("test_return:", a, b, c)
	return a, b, c
end 

local a, b, c, d = test_return(1, 2, 3)
print(a, b, c, d)

function test_return_call(a, b, c)
	return 1, test_return(a, b, c), test_return(a, b, c)
end 

d, e, f, g, h = test_return_call(4, 5, 6)
print(d, e, f, g, h)

-- local a, b, c = 10, 30, 20
-- if a >= b then
-- 	print("a>=b")
-- elseif a <= c then
-- 	print("a<=c")
-- else
-- 	print("else-part")
-- end