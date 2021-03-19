-- test case:__index is a table
print("----------test case 1----------")
local mt = { [1] = 2020 }
local tbl = setmetatable({}, { __index = mt })
print(tbl[1])
print(tbl[2])

-- test case2
print("----------test case 2----------")
local c2_mt0 = { world = "hello" }
local c2_mt1 = { hello = "world" }
setmetatable(c2_mt1, { __index = c2_mt0 })

local c2_tbl = setmetatable({}, { __index = c2_mt1 })
print(c2_tbl.world)
print(c2_tbl.hello)
print(c2_tbl.xxx)
print(c2_mt1.hello)
print(c2_mt1.world)
print(c2_mt0.hello)
print(c2_mt0.world)

-- test case3
print("----------test case 3----------")
local c3_mt0 = setmetatable({}, { __index = function(t, k) print(t, k) end })
local c3_mt1 = setmetatable({}, { __index = c3_mt0 })
local c3_tbl = setmetatable({}, { __index = c3_mt1 })

print(c3_mt1.hello)
print(c3_tbl.hello)

-- test case4
print("----------test case 4----------")
local c4_mt0 = setmetatable({}, { __index = function(t, k) print("c4_mt0") end })
local c4_mt1 = setmetatable({}, { __index = function(t, k) print("c4_mt1"); return "hello world" end })
local c4_tbl = setmetatable({}, { __index = c4_mt1 })

print(c4_mt1.hello)
print(c4_tbl.hello)

-- test case5
print("----------test case 5----------")
local c5_mt0 = setmetatable({}, { __newindex = function(t, k, v) print("c5_mt0"); print(t, k, v) end })
local c5_mt1 = setmetatable({}, { __newindex = function(t, k, v) print("c5_mt1"); print(t, k, v) end })
local c5_tbl = setmetatable({}, { __newindex = c5_mt1})

c5_tbl.key = "key"
print(c5_tbl.key)

c5_mt1.key = "key_c5_mt1"
print(c5_tbl.key)

-- test case6
print("----------test case 6----------")
local c6_mt0 = {}
local c6_mt1 = setmetatable({}, { __newindex = c6_mt0 })
local c6_mt2 = setmetatable({}, { __newindex = c6_mt1 })

c6_mt2.key = "key1"
print(c6_mt2.key)
print(c6_mt1.key)
print(c6_mt0.key)

c6_mt1.key = "key2"
print(c6_mt0.key)

-- test case7
print("----------test case 7----------")
local t1, t2 = {}, {}
setmetatable(t1, { __add = function(lhs, rhs)  print(lhs, rhs); return 0 end })
local t3 = t1 + t2
print(t3)

-- ignore more binary arith test cases

-- test case8
print("----------test case 8----------")
local c8_t1 = { value = 1 }
local c8_t2 = { value = 2 }
setmetatable(c8_t1, { 
	__eq = function(lhs, rhs) print("__eq"); return lhs.value == rhs.value end,
	__lt = function(lhs, rhs) print("__lt"); return lhs.value < rhs.value end,
	__le = function(lhs, rhs) print("__le"); return lhs.value <= rhs.value end,
	__concat = function(lhs, rhs) return tostring(lhs.value) .. tostring(rhs.value) end,
	})
print(c8_t1 == c8_t2)
print(c8_t1 < c8_t2)
print(c8_t1 <= c8_t2)
print(c8_t1 .. c8_t2)