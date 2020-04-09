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

math.randomseed(os.time());
math.random(); math.random(); math.random();

local t=Time(true);
Sleep(123);
print(Time(true)-t)

local function test()
	local info=FileSystem.GetFileInfo("D:/pics.hak");if not info then return -1 else return info.Size; end
end

print("Percent                    ", GlobalMemoryStatus());
print("total KB of physical memory", GlobalMemoryStatus(1));
print("free  KB of physical memory", GlobalMemoryStatus(2));
print("total KB of paging file    ", GlobalMemoryStatus(3));
print("free  KB of paging file    ", GlobalMemoryStatus(4));
print("total KB of virtual memory ", GlobalMemoryStatus(5));
print("free  KB of virtual memory ", GlobalMemoryStatus(6));
print("Lua memory                 ", collectgarbage("count"));
print("FileSize                   ", test());
print("setpause                   ", collectgarbage('setpause', 150));
print("setstepmul                 ", collectgarbage('setstepmul', 250));

local function SetGCFunction(tbl, func)
	return setmetatable(tbl, {__gc = func})
end

local function CreateGCPrint()
	SetGCFunction({}, function() print("COLLECTING GARBAGE Lua mem: "..math.floor(collectgarbage("count")) .. " KB Time: "..Time()); CreateGCPrint(); end);
end
CreateGCPrint();
collectgarbage();

local arr = {}
for n=1, 7 do 
	table.insert(arr,GlobalMemoryStatus(n));
end

local NULL = Json.GetNull();

local test2 = {Kek=print, blarg=123, float=math.pi, t=true, f=false, n=Json.GetNull()};
local test = {Inf=math.huge, Test="123\0", "Meow", Burk=123, Bake=test2, Mems=arr, Empty={}, obj=Json.GetEmpty()}

local res = Json.Encode(test);
print(res);
local newtest = Json.Decode(res);
TablePrint(newtest);
local function CheckIsEqual(test, test2)

	for k,v in pairs(test) do 
		
		local op = test2[k];

		if v == NULL then v=nil; end
		if op == NULL then op=nil; end

		if type(v) ~= type(op) then
			print(tostring(k).." "..type(v).." not equal type as "..type(op));
		elseif type(v) == "table" then 
			CheckIsEqual(v, op);
		elseif v ~= op then 
			print(tostring(k).." "..tostring(v).." not equal "..tostring(op));
		end
	end
end

CheckIsEqual(newtest, test);
CheckIsEqual(test, newtest);

--FileSystem.SetCurrentDirectory("C:\\Users\\Terrah\\Desktop\\TwitchToKafkaAdminer");
--dofile("C:\\Users\\Terrah\\Desktop\\TwitchToKafkaAdminer\\main.lua");