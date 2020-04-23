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

local s = Stream.Create();

s:Buffer("Hello this is text\0");
print(s:IndexOf(0));
print(s:IndexOf("is"));
print(s:ReadUntil(string.byte("i")));
s:Read(1);
print(s:ReadUntil(string.byte("i")));
s:Read(1);
print(s:ReadUntil(string.byte("i")));
print(s:Read());
print(s:Read());

if s then return; end

local j = Json.Create();
local db=assert(MySQL.Connect("10.9.23.252", "TwitchKafka", "meowCat69!", "twitch"));

print(j:Encode("hello"));
Break();

j:EncodeToFile("d:/big.json", coroutine.create(function() 

	assert(db:Query("SELECT * FROM eventlogs LIMIT 800000;"));
	coroutine.yield(nil, {});

	while db:Fetch() do

		coroutine.yield(1, {});

		for k,v in pairs(db:GetRow()) do 
			coroutine.yield(k, v);
		end

		coroutine.yield(nil, nil);
	end

	coroutine.yield(nil, nil);
end));

local data={};
local str,f;
while true do  
	
	print("DecodeFromFile");
	data = j:DecodeFromFile("d:/big.json");
	print("Encode");
	str = j:Encode(data);
	f=io.open("d:/big2.json", "w");
	f:write(str);
	f:flush();
	f:close();
	data = nil;
	str = nil;
	f=io.open("d:/big2.json", "r");

	print("DecodeFromFunction");
	data = j:DecodeFromFunction(function() return f:read(1000); end);
	f:close();

	f=Stream.Create();
	j:EncodeToFunction(function(part) f:Buffer(part); end, data);
	data=nil;

	data = j:DecodeFromFunction(function() return f:Read(1000); end);
	f=nil;
	data=nil;

	f=io.open("d:/big2.json", "r");
	str = f:read("*all");
	f:close();

	print("Decode");
	data = j:Decode(str);
	str=nil;

	print("EncodeToFile");
	j:EncodeToFile("d:/big3.json", data);
	data=nil;

	Break();
end