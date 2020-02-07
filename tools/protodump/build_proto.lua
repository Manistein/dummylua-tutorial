-- author:Manistein
-- desc:A tool to show lua bytecode proto
-- date:2018-08-25

local file_name, output_file_name = ...
print(string.format("file_name = %s", file_name))

local define = {}
define.size = {
    int = 0,
    size_t = 0,
    instruction = 0,
    lua_integer = 0,
    lua_number = 0,
}
define.signature = "\x1bLua"
define.signature_hex_str = "1b4c7561"
define.luac_data = "\x19\x93\r\n\x1a\n"
define.version = 0x53
define.official = 0x00
define.luac_num = 370.5
define.big_endian_code = 0x5678
define.endianess = 0 -- 1 is little endian

-------------------------------------------------------------------------------------
-- opcode define
-------------------------------------------------------------------------------------
local optype = {
    iABC     = 0,
    iABx     = 1,
    iAsBx    = 2,
    iAx      = 3,
}

local opargmask = {
    OpArgN   = 0,
    OpArgU   = 1,
    OpArgK   = 2,
    OpArgR   = 3,
}

local opname2code = {
-------------------------------------------------------------------------
-- name                         args    description
------------------------------------------------------------------------
OP_MOVE             = 0,     -- A B     R(A) := R(B)                    
OP_LOADK            = 1,     -- A Bx    R(A) := Kst(Bx)                 
OP_LOADKX           = 2,     -- A       R(A) := Kst(extra arg)              
OP_LOADBOOL         = 3,     -- A B C   R(A) := (Bool)B; if (C) pc++            
OP_LOADNIL          = 4,     -- A B     R(A), R(A+1), ..., R(A+B) := nil        
OP_GETUPVAL         = 5,     -- A B     R(A) := UpValue[B]              

OP_GETTABUP         = 6,     -- A B C   R(A) := UpValue[B][RK(C)]           
OP_GETTABLE         = 7,     -- A B C   R(A) := R(B)[RK(C)]             

OP_SETTABUP         = 8,     -- A B C   UpValue[A][RK(B)] := RK(C)          
OP_SETUPVAL         = 9,     -- A B     UpValue[B] := R(A)              
OP_SETTABLE         = 10,    -- A B C   R(A)[RK(B)] := RK(C)                

OP_NEWTABLE         = 11,    -- A B C   R(A) := {} (size = B,C)             

OP_SELF             = 12,    -- A B C   R(A+1) := R(B); R(A) := R(B)[RK(C)]     

OP_ADD              = 13,    -- A B C   R(A) := RK(B) + RK(C)               
OP_SUB              = 14,    -- A B C   R(A) := RK(B) - RK(C)               
OP_MUL              = 15,    -- A B C   R(A) := RK(B) * RK(C)               
OP_MOD              = 16,    -- A B C   R(A) := RK(B) % RK(C)               
OP_POW              = 17,    -- A B C   R(A) := RK(B) ^ RK(C)               
OP_DIV              = 18,    -- A B C   R(A) := RK(B) / RK(C)               
OP_IDIV             = 19,    -- A B C   R(A) := RK(B) // RK(C)              
OP_BAND             = 20,    -- A B C   R(A) := RK(B) & RK(C)               
OP_BOR              = 21,    -- A B C   R(A) := RK(B) | RK(C)               
OP_BXOR             = 22,    -- A B C   R(A) := RK(B) ~ RK(C)               
OP_SHL              = 23,    -- A B C   R(A) := RK(B) << RK(C)              
OP_SHR              = 24,    -- A B C   R(A) := RK(B) >> RK(C)              
OP_UNM              = 25,    -- A B     R(A) := -R(B)                   
OP_BNOT             = 26,    -- A B     R(A) := ~R(B)                   
OP_NOT              = 27,    -- A B     R(A) := not R(B)                
OP_LEN              = 28,    -- A B     R(A) := length of R(B)              

OP_CONCAT           = 29,    -- A B C   R(A) := R(B).. ... ..R(C)           

OP_JMP              = 30,    -- A sBx   pc+=sBx; if (A) close all upvalues >= R(A - 1)  
OP_EQ               = 31,    -- A B C   if ((RK(B) == RK(C)) ~= A) then pc++        
OP_LT               = 32,    -- A B C   if ((RK(B) <  RK(C)) ~= A) then pc++        
OP_LE               = 33,    -- A B C   if ((RK(B) <= RK(C)) ~= A) then pc++        

OP_TEST             = 34,    -- A C     if not (R(A) <=> C) then pc++           
OP_TESTSET          = 35,    -- A B C   if (R(B) <=> C) then R(A) := R(B) else pc++ 

OP_CALL             = 36,    -- A B C   R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) 
OP_TAILCALL         = 37,    -- A B C   return R(A)(R(A+1), ... ,R(A+B-1))      
OP_RETURN           = 38,    -- A B     return R(A), ... ,R(A+B-2)  (see note)  

OP_FORLOOP          = 39,    -- A sBx   R(A)+=R(A+2);
                             --         if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
OP_FORPREP          = 40,    -- A sBx   R(A)-=R(A+2); pc+=sBx               

OP_TFORCALL         = 41,    -- A C     R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));  
OP_TFORLOOP         = 42,    -- A sBx   if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }

OP_SETLIST          = 43,    -- A B C   R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B    

OP_CLOSURE          = 44,    -- A Bx    R(A) := closure(KPROTO[Bx])         

OP_VARARG           = 45,    -- A B     R(A), R(A+1), ..., R(A+B-2) = vararg        

OP_EXTRAARG         = 46,    -- Ax      extra (larger) argument for previous opcode 
}

local opcode2name = {}
for k, v in pairs(opname2code) do 
    opcode2name[v] = k
end 

local function opmode(t,a,b,c,m)
    return (((t)<<7) | ((a)<<6) | ((b)<<4) | ((c)<<2) | (m))
end 

local opmode = {
  opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iABC)        -- OP_MOVE 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgN, optype.iABx)        -- OP_LOADK 
 ,opmode(0, 1, opargmask.OpArgN, opargmask.OpArgN, optype.iABx)        -- OP_LOADKX 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgU, optype.iABC)        -- OP_LOADBOOL 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgN, optype.iABC)        -- OP_LOADNIL 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgN, optype.iABC)        -- OP_GETUPVAL 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgK, optype.iABC)        -- OP_GETTABUP 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgK, optype.iABC)        -- OP_GETTABLE 
 ,opmode(0, 0, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_SETTABUP 
 ,opmode(0, 0, opargmask.OpArgU, opargmask.OpArgN, optype.iABC)        -- OP_SETUPVAL 
 ,opmode(0, 0, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_SETTABLE 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgU, optype.iABC)        -- OP_NEWTABLE 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgK, optype.iABC)        -- OP_SELF 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_ADD 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_SUB 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_MUL 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_MOD 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_POW 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_DIV 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_IDIV 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_BAND 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_BOR 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_BXOR 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_SHL 
 ,opmode(0, 1, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_SHR 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iABC)        -- OP_UNM 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iABC)        -- OP_BNOT 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iABC)        -- OP_NOT 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iABC)        -- OP_LEN 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgR, optype.iABC)        -- OP_CONCAT 
 ,opmode(0, 0, opargmask.OpArgR, opargmask.OpArgN, optype.iAsBx)       -- OP_JMP 
 ,opmode(1, 0, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_EQ 
 ,opmode(1, 0, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_LT 
 ,opmode(1, 0, opargmask.OpArgK, opargmask.OpArgK, optype.iABC)        -- OP_LE 
 ,opmode(1, 0, opargmask.OpArgN, opargmask.OpArgU, optype.iABC)        -- OP_TEST 
 ,opmode(1, 1, opargmask.OpArgR, opargmask.OpArgU, optype.iABC)        -- OP_TESTSET 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgU, optype.iABC)        -- OP_CALL 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgU, optype.iABC)        -- OP_TAILCALL 
 ,opmode(0, 0, opargmask.OpArgU, opargmask.OpArgN, optype.iABC)        -- OP_RETURN 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iAsBx)       -- OP_FORLOOP 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iAsBx)       -- OP_FORPREP 
 ,opmode(0, 0, opargmask.OpArgN, opargmask.OpArgU, optype.iABC)        -- OP_TFORCALL 
 ,opmode(0, 1, opargmask.OpArgR, opargmask.OpArgN, optype.iAsBx)       -- OP_TFORLOOP 
 ,opmode(0, 0, opargmask.OpArgU, opargmask.OpArgU, optype.iABC)        -- OP_SETLIST 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgN, optype.iABx)        -- OP_CLOSURE 
 ,opmode(0, 1, opargmask.OpArgU, opargmask.OpArgN, optype.iABC)        -- OP_VARARG 
 ,opmode(0, 0, opargmask.OpArgU, opargmask.OpArgU, optype.iAx)         -- OP_EXTRAARG 
}

local temp = opmode 
opmode = {}
for k, v in ipairs(temp) do 
    opmode[k - 1] = v
end  

local opname2desc = {
OP_MOVE             = "R(A) := R(B)",
OP_LOADK            = "R(A) := Kst(Bx)",
OP_LOADKX           = "R(A) := Kst(extra arg)",
OP_LOADBOOL         = "R(A) := (Bool)B; if (C) pc++",
OP_LOADNIL          = "R(A), R(A+1), ..., R(A+B) := nil",
OP_GETUPVAL         = "R(A) := UpValue[B]",
OP_GETTABUP         = "R(A) := UpValue[B][RK(C)]",
OP_GETTABLE         = "R(A) := R(B)[RK(C)]",
OP_SETTABUP         = "UpValue[A][RK(B)] := RK(C)",
OP_SETUPVAL         = "UpValue[B] := R(A)",
OP_SETTABLE         = "R(A)[RK(B)] := RK(C)",
OP_NEWTABLE         = "R(A) := {} (size = B,C)",
OP_SELF             = "R(A+1) := R(B); R(A) := R(B)[RK(C)]",
OP_ADD              = "R(A) := RK(B) + RK(C)",
OP_SUB              = "R(A) := RK(B) - RK(C)",
OP_MUL              = "R(A) := RK(B) * RK(C)",
OP_MOD              = "R(A) := RK(B) % RK(C)",
OP_POW              = "R(A) := RK(B) ^ RK(C)",
OP_DIV              = "R(A) := RK(B) / RK(C)",
OP_IDIV             = "R(A) := RK(B) // RK(C)",
OP_BAND             = "R(A) := RK(B) & RK(C)",
OP_BOR              = "R(A) := RK(B) | RK(C)",
OP_BXOR             = "R(A) := RK(B) ~ RK(C)",
OP_SHL              = "R(A) := RK(B) << RK(C)",
OP_SHR              = "R(A) := RK(B) >> RK(C)",
OP_UNM              = "R(A) := -R(B)",
OP_BNOT             = "R(A) := ~R(B)",
OP_NOT              = "R(A) := not R(B)",
OP_LEN              = "R(A) := length of R(B)",
OP_CONCAT           = "R(A) := R(B).. ... ..R(C)",
OP_JMP              = "pc+=sBx; if (A) close all upvalues >= R(A - 1)",
OP_EQ               = "if ((RK(B) == RK(C)) ~= A) then pc++",
OP_LT               = "if ((RK(B) <  RK(C)) ~= A) then pc++",
OP_LE               = "if ((RK(B) <= RK(C)) ~= A) then pc++",
OP_TEST             = "if not (R(A) <=> C) then pc++",
OP_TESTSET          = "if (R(B) <=> C) then R(A) := R(B) else pc++ ",
OP_CALL             = "R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))",
OP_TAILCALL         = "return R(A)(R(A+1), ... ,R(A+B-1))",
OP_RETURN           = "return R(A), ... ,R(A+B-2)  (see note)",
OP_FORLOOP          = "R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }",
OP_FORPREP          = "R(A)-=R(A+2); pc+=sBx",
OP_TFORCALL         = "R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));",
OP_TFORLOOP         = "if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }",
OP_SETLIST          = "R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B",
OP_CLOSURE          = "R(A) := closure(KPROTO[Bx])",
OP_VARARG           = "R(A), R(A+1), ..., R(A+B-2) = vararg",
OP_EXTRAARG         = "extra (larger) argument for previous opcode",
}

local function get_type_mode(modecode)
    return modecode & 0x3
end 

local function get_b_mode(modecode)
    return (modecode >> 4 & 0x03)
end 

local function get_c_mode(modecode)
    return (modecode >> 2 & 0x03)
end 

local function get_opcode(instruction)
    return instruction & 0x3f
end 

local function get_a_var(instruction)
    return (instruction >> 6) & 0xff
end 

local function get_b_var(instruction)
    return instruction >> 23  
end 

local function get_c_var(instruction)
    return (instruction >> 14) & 0x1ff
end 

local function get_bx_var(instruction)
    return instruction >> 14
end 

local function get_sbx_var(instruction)
    return instruction >> 14
end 

local function append_opname_space(opname)
    local max_opname_len = 16
    local opname_len = string.len(opname)
    
    local space = ""
    for i = 1, max_opname_len - opname_len do 
        space = space .. " "
    end 
    return space 
end 

local function append_var_space(var)
    local min_var_len = 3
    local var_str = tostring(var)
    
    local ret = ""
    for i = 1, min_var_len - string.len(var_str) do 
        ret = ret .. " "
    end 
    return ret 
end 

local function iabc2str(instruction)
    local opcode = get_opcode(instruction)
    local mode = get_type_mode(opmode[opcode])  
    assert(mode == optype.iABC)
    
    local ret = "iABC:"
    local opname = opcode2name[opcode]
    ret = ret .. opname .. append_opname_space(opname) .. " "

    local avar = get_a_var(instruction)
    ret = ret .. "A:" ..  avar .. append_var_space(avar) .. " " 

    local bmode = get_b_mode(opmode[opcode])
    if bmode ~= opargmask.OpArgN then 
        local bvar = get_b_var(instruction)
        ret = ret .. "B  :" .. bvar .. append_var_space(bvar) .. " " 
    else
        ret = ret .. "      "
    end  

    local cmode = get_c_mode(opmode[opcode])
    if cmode ~= opargmask.OpArgN then 
        local cvar = get_c_var(instruction)
        ret = ret .. "C:" .. cvar .. append_var_space(cvar) .. " "
    else 
        ret = ret .. "      "
    end   

    ret = ret .. "; " .. opname2desc[opname]

    return ret 
end 

local function iabx2str(instruction)
    local opcode = get_opcode(instruction)
    local mode = get_type_mode(opmode[opcode])
    assert(mode == optype.iABx)

    local ret = "iABx:"
    local opname = opcode2name[opcode]
    ret = ret .. opname .. append_opname_space(opname) ..  " "

    local avar = get_a_var(instruction)
    ret = ret .. "A:" .. avar .. append_var_space(avar) .. " "

    local bmode = get_b_mode(opmode[opcode])
    if bmode ~= opargmask.OpArgN then 
        local bxvar = get_bx_var(instruction)
        ret = ret .. "Bx :" .. bxvar .. append_var_space(bxvar) ..  "       " 
    else
        ret = ret .. "            "
    end  
    
    ret = ret .. "; " .. opname2desc[opname]
    
    return ret
end 

local function iasbx2str(instruction)
    local opcode = get_opcode(instruction)
    local mode = get_type_mode(opmode[opcode])
    assert(mode == optype.iAsBx)

    local ret = "iAsBx:"
    local opname = opcode2name[opcode]
    ret = ret .. opname .. append_opname_space(opname) .. " "
    
    local avar = get_a_var(instruction)
    ret = ret .. "A:" .. avar  .. append_var_space(avar) .. " "

    local sbxvar = get_sbx_var(instruction)
    ret = ret .. "sBx:" .. sbxvar .. append_var_space(sbxvar) .. "       " 

    ret = ret .. "; " .. opname2desc[opname]

    return ret
end 

-------------------------------------------------------------------------------------
-- Load and Parse file
-------------------------------------------------------------------------------------
local load_index = 1 
local load_function = nil

local function str2hex(str)
    if define.endianess == 0 then 
        return tonumber(str, 16)
    end 

    local reverse_str = ""
    local reverse_tbl = {}
    for idx = 1, string.len(str), 2 do 
        local element = string.sub(str, idx, idx + 1)
        table.insert(reverse_tbl, 1, element)
    end 
    
    for k, v in ipairs(reverse_tbl) do 
        reverse_str = reverse_str .. v
    end 

    return tonumber(reverse_str, 16)
end

local function hex2decimal(value)
    return tonumber(tostring(value), 10)
end 

-- base load function 
local function load_byte(code_str)
    local two_char = string.sub(code_str, load_index, load_index + 1)
    load_index = load_index + 2 -- 2 chars represent a hex
    return two_char
end 

local function load_int(code_str)
    local ret = ""
    for i = 1, define.size.int do 
        ret = ret .. load_byte(code_str)
    end 
    return ret 
end 

local function load_integer(code_str)
    local ret = ""

    for i = 1, define.size.lua_integer do 
        ret = ret .. load_byte(code_str)
    end 

    return ret 
end 

local function load_number(code_str)
    local ret = ""
    
    for i = 1, define.size.lua_number do 
        ret = ret .. load_byte(code_str)
    end     

    return ret 
end 

local function load_string(code_str)
    local size = str2hex(load_byte(code_str))
    if size == 0xFF then 
        size = hex2decimal(str2hex(load_number(code_str)))
    end 

    if size == 0 then 
        return nil 
    end 

    local str_len = hex2decimal(size)
    local ret = ""
    
    for i = 1, str_len - 1 do 
        local byte = load_byte(code_str)
        local byte_hex = str2hex(byte)
        local byte_decimal = hex2decimal(byte_hex)
        ret = ret .. string.char(byte_decimal)
    end 

    return ret  
end 

local function check_header(code_str)
    -- check signature
    local sinature_str = ""
    for i = 1, 4 do 
        sinature_str = sinature_str .. load_byte(code_str)
    end

    if sinature_str ~= define.signature_hex_str then 
        assert(false, string.format("sinature %s not match \x1bLua", sinature_str))
    end 

    -- check version
    local version = tonumber((load_byte(code_str)), 16)
    if version ~= define.version then 
        assert(false, string.format("lua version number is mismatch"))    
    end 

    -- check is official
    local official = tonumber((load_byte(code_str)), 16)
    if official ~= define.official then 
        assert(false, string.format("bin file is not official version"))
    end 

    -- skip LUAC_DATA bytes
    for i = 1, 6 do 
        load_byte(code_str)
    end 

    -- size int
    define.size.int = assert(tonumber(load_byte(code_str)))
    -- size size_t
    define.size.size_t = assert(tonumber(load_byte(code_str)))
    -- size instruction
    define.size.instruction = assert(tonumber(load_byte(code_str)))
    -- size lua_integer
    define.size.lua_integer = assert(tonumber(load_byte(code_str)))
    -- size lua_number 
    define.size.lua_number = assert(tonumber(load_byte(code_str)))

    -- check endian
    local endian = tonumber((load_integer(code_str)), 16)
    if define.big_endian_code ~= endian then 
        define.endianess = 1
    else
        define.endianess = 0
    end 

    -- skip check lua_number
    local luac_num = hex2decimal(str2hex(load_integer(code_str)))
    --print("luac_num " .. luac_num)
end 

-- loader
local function load_code(code_str)
    local code = {}
    
    local ncode = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, ncode do 
       local instruction = hex2decimal(str2hex(load_int(code_str))) 
       local opcode = get_opcode(instruction)
       local mode = get_type_mode(opmode[opcode])
       if mode == optype.iABC then 
            code[i] = iabc2str(instruction)
       elseif mode == optype.iABx then 
            code[i] = iabx2str(instruction)
       elseif mode == optype.iAsBx then  
            code[i] = iasbx2str(instruction)
       elseif mode == optype.iAx then 
            print("load_code mode iAx todo")
       else
            assert(false) 
       end 
    end 

    return code 
end 

local function load_const(code_str)
    local constants = {}    

    local nconst = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nconst do 
        local const_type = hex2decimal(str2hex(load_byte(code_str))) 
        if const_type == 0 then  -- LUA_TNIL
           constants[i] = "nil"
        elseif const_type == 1 then -- LUA_TBOOLEAN 
           constants[i] = "boolean:" .. hex2decimal(str2hex(load_byte(code_str)))
        elseif const_type == 3 then -- LUA_TNUMFLT
           constants[i] = "float:" .. hex2decimal(str2hex(load_number(code_str))) 
        elseif const_type == 19 then -- LUA_TINT
           constants[i] = "int:" .. hex2decimal(str2hex(load_integer(code_str))) 
        elseif const_type == 4 or const_type == 20 then -- string
           constants[i] = load_string(code_str)
        else 
            assert(false)
        end 
    end 

    return constants
end 

local function load_upvalues(code_str)
    local upvalues = {}

    local nupvalues = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nupvalues do 
        local upvalue = {}
        upvalue.instack = hex2decimal(str2hex(load_byte(code_str)))
        upvalue.idx = hex2decimal(str2hex(load_byte(code_str)))        

        upvalues[i] = upvalue 
    end 

    return upvalues     
end 

local function load_protos(code_str)
    local protos = {}

    local nproto = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nproto do 
        protos[i] = load_function(code_str)
    end 
    
    return protos    
end 

local function load_debug(code_str)
    local dbg = {}
    dbg.lineinfo = {}

    local nlineinfo = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nlineinfo do 
        local lineinfo = hex2decimal(str2hex(load_int(code_str)))
        table.insert(dbg.lineinfo, lineinfo)
    end 

    dbg.locvars = {}
    local nvars = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nvars do 
        local var = {}
        var.varname = load_string(code_str)
        var.startpc = hex2decimal(str2hex(load_int(code_str)))
        var.endpc   = hex2decimal(str2hex(load_int(code_str)))
        
        dbg.locvars[i] = var
    end 

    dbg.upvalues = {}
    local nupvalues = hex2decimal(str2hex(load_int(code_str)))
    for i = 1, nupvalues do 
        dbg.upvalues[i] = load_string(code_str)
    end 

    return dbg  
end 

function load_function(code_str)
    local proto = {}
    
    proto.type_name = "Proto"
    proto.source = load_string(code_str)     
    proto.linedefine = hex2decimal(str2hex(load_int(code_str)))
    proto.lastlinedefine = hex2decimal(str2hex(load_int(code_str)))
    proto.numparams = hex2decimal(str2hex(load_byte(code_str)))
    proto.is_vararg = hex2decimal(str2hex(load_byte(code_str)))
    proto.maxstacksize = hex2decimal(str2hex(load_byte(code_str)))
    proto.code = load_code(code_str)
    proto.k = load_const(code_str)
    proto.upvalues = load_upvalues(code_str)
    proto.p = load_protos(code_str)
    
    local dbg = load_debug(code_str)
    proto.lineinfo = dbg.lineinfo 
    proto.locvars = dbg.locvars 
    for idx, v in pairs(dbg.upvalues) do 
        proto.upvalues[idx].name = v 
    end 

    return proto    
end 

local function new_lclosure(nupvalues)
    local ret = {}
    ret.type_name = "LClosure"
    ret.upvals = {}

    for i = 1, nupvalues do 
        local upval = {}
        table.insert(ret.upvals, upval)
        
        if i == 1 then 
            upval.name = "_ENV"
        end 
    end 

    ret.proto = {}

    return ret 
end 

---------------------------------------------------------------------------------------------
-- print function
---------------------------------------------------------------------------------------------

local function dump(root)
    if root == nil then
        return error("PRINT_T root is nil")
    end
    if type(root) ~= type({}) then
        return error("PRINT_T root not table type")
    end
    if not next(root) then
        return error("PRINT_T root is space table")
    end

    local cache = { [root] = "." }
    local function _dump(t,space,name)
        local temp = {}
        for k,v in pairs(t) do
            local key = tostring(k)
            if cache[v] then
                table.insert(temp,"+" .. key .. " {" .. cache[v].."}")
            elseif type(v) == "table" then
                local new_key = name .. "." .. key
                cache[v] = new_key
                table.insert(temp,"+" .. key .. _dump(v,space .. (next(t,k) and "|" or " " ).. string.rep(" ",#key),new_key))
            else
                table.insert(temp,"+" .. key .. " [" .. tostring(v).."]")
            end
        end
        return table.concat(temp,"\n"..space)
    end
    print(_dump(root, "",""))
end

-----------------------------------------------------------------------------------
-- main entry
-----------------------------------------------------------------------------------

local function get_pure_code()
    local file_handle = io.open(file_name, "r");
    
    local code_str = ""
    local line = file_handle:read("l")
    while line do 
        code_str = code_str .. line
        line = file_handle:read("l")
    end 

    file_handle:close()

    return code_str
end 

-- entry
local function main()
    local code_str = get_pure_code()
    
    check_header(code_str) 
    local lclosure = new_lclosure(hex2decimal(str2hex(load_byte(code_str))))
    lclosure.proto = load_function(code_str)

    dump(lclosure)
end 

main()
