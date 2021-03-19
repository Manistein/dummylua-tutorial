-- test case 1
do 
	local var1 = 1
	function aaa()
		local var2 = 1
		function bbb()
			var1 = var1 + 1
			var2 = var2 + 1
			print("bbb:", var1, var2)
		end 

		function ccc()
			var1 = var1 + 1
			var2 = var2 + 1
			print("ccc:", var1, var2)
		end

		bbb()
		print("hahaha")
	end

	aaa()
	bbb()
	bbb()
	ccc()
	ccc()
end

-- test case 2
do 
	local var1 = 1
	local function aaa()
		local var2 = 1
		return function()
			var1 = var1 + 1
			var2 = var2 + 1

			print(var1, var2)
		end
	end

	local f1 = aaa()
	local f2 = aaa()

	f1()
	f2()
end

-- test case 3
do
	local var1 = 1
	local function aaa()
		local var2 = {}
		var2.value = 1

		return function()
			var1 = var1 + 1
			var2.value = var2.value + 1
			print(var1, var2.value)
		end
	end

	local f1 = aaa()
	local f2 = aaa()
	f1()
	f2()
end