local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

JSON = assert(loadfile "JSON.lua")();
function TablePrint(tbl, depth)

	if(not tbl and depth) then
		assert(tbl, depth);
	end

	depth = depth or 0;

	local padding="";

	for n=1, depth do 
		padding = padding.." ";
	end

	print(padding..tostring(tbl));

	if type(tbl)~="table" then 	
		return;
	end

	for k,v in pairs(tbl) do 
		print(padding..tostring(k)..": "..tostring(v));

		if type(v)=="table" then 
			TablePrint(v, depth+1);
		end
	end

end

function ArrayPrint(arr)

	print(tostring(arr).." "..tostring(#arr));

	if type(arr)~="table" then 
		return;
	end

	for n=1,#arr do 
		print(arr[n]);
	end 
end

for n=1, #ARGS do 
	print(n, ARGS[n]);
end

function PrintPixel(px)
	io.write("{"..px.r.." | ");
	io.write(px.g .. " | ");
	print(px.b .. "} ");
end

function DumpToFile(file, tbl)
	local f = io.open(file, "w");
	f:write(JSON:encode_pretty(tbl));
	f:flush();
	f:close();
end

local statusTimer = Timer.New();
statusTimer:Start();
function WriteStatusString(str, prevlen, sincelast)

	if statusTimer:Elapsed() <= sincelast then 
		return prevlen;
	else 
		statusTimer:Stop();
		statusTimer:Reset();
		statusTimer:Start();
	end

	prevlen = prevlen or 0;

	if prevlen > 0 then 

		for n=1, prevlen do 
			io.write("\b");
		end 

		for n=1, prevlen do 
			io.write(" ");
		end 

		for n=1, prevlen do 
			io.write("\b");
		end 
	end

	io.write(str);

	return str:len();
end

function string.fromhex(str)
    return (str:gsub('..', function (cc)
        return string.char(tonumber(cc, 16))
    end))
end

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end

math.randomseed(Time());
math.random(); math.random(); math.random();

print("Percent                    ", GlobalMemoryStatus());
print("total KB of physical memory", GlobalMemoryStatus(1));
print("free  KB of physical memory", GlobalMemoryStatus(2));
print("total KB of paging file    ", GlobalMemoryStatus(3));
print("free  KB of paging file    ", GlobalMemoryStatus(4));
print("total KB of virtual memory ", GlobalMemoryStatus(5));
print("free  KB of virtual memory ", GlobalMemoryStatus(6));

local function SetGCFunction(tbl, func)
	return setmetatable(tbl, {__gc = func})
end

local function CreateGCPrint()
	SetGCFunction({last=Time()}, function(obj) local t=Time();print("COLLECTING GARBAGE Lua mem: "..math.floor(collectgarbage("count")) .. " KB Time: "..(t-obj.last)); CreateGCPrint(); end);
end
CreateGCPrint();
collectgarbage();

--[[local folders = {};
folders["AuroraEngine"] = "D:/AuroraEngine";
folders["isos"] = "D:/isos";

local keybif = KeyBif.Create("R:/Test.key", folders);]]

local btree = BinaryTree.Create();
print(btree);
print(btree:Count());
print("Add",btree:Add(5, true));
print("Add",btree:Add(3, "abc"));
print("Add",btree:Add(2, 5));
print("Add",btree:Add(1, "123"));
print("Add",btree:Add(3, "abc"));
print(btree:Count());

print("-----");
btree:Iterate(function(k,v) print(k,v); end);
print("-----");
btree:Iterate(function(k,v) print(k,v); end, 1);
print("-----");
btree:Iterate(function(k,v) print(k,v); end, 2);


print("Get", btree:Get(3));
print("Get", btree:Get(2));
print("Get", btree:Get(1));
print("fail", btree:Get(4));

print("Delete", btree:Delete(3));
print("Delete", btree:Delete(2));
print("Delete", btree:Delete(1));
print("Delete", btree:Delete(5));

print("-----");
btree:Iterate(function(k,v) print(k,v); end);

for n=1, 10 do 
	print(n, "Add",btree:Add(n, "Meow"..tostring(n)));
end

print("-----");
btree:Iterate(function(k,v) print(k,v); end);

print("Delete", btree:Delete(1));
print("-----");
btree:Iterate(function(k,v) print(k,v); end);

btree = BinaryTree.Create();
btree:Add(1, "Hihihi");

for n=1, 20000000 do 
	btree:Add(CRC32(n), n);
end
btree:Add(2, "Last");

print("-----");
--btree:Iterate(function(k,v) print(k,v); end);
print("Get", btree:Get(1));
print("Get", btree:Get(2));


print(btree:Count());