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

local function utf8char(utf)

	if utf <= 0xFF  then

		return string.char(utf);

	elseif utf <= 0xFFFF then

		return string.char((utf & 0x0000FF00) >> 8, utf & 0x000000FF);

	elseif utf <= 0xFFFFFF then

		return string.char((utf & 0x00FF0000) >> 16,(utf & 0x0000FF00) >> 8, utf & 0x000000FF);

	elseif utf <= 0xFFFFFFFF then

		return string.char((utf & 0xFF000000) >> 24, (utf & 0x00FF0000) >> 16,(utf & 0x0000FF00) >> 8, utf & 0x000000FF);
	else 
		return nil;
	end
end

local function ReadUTF8(stream)
	
	local avail = stream:len() - stream:pos();

	if avail <= 0 then 
		return nil;
	end 

	local byte = stream:PeekByte();
	local result = 0;

	if byte > 0x10000 then 

		if avail < 4 then 
			return nil;
		end
		
		result = stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();

	elseif byte > 0x800 then 

		if avail < 3 then 
			return nil;
		end
		
		result = stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();

	elseif byte > 0x80 then 

		if avail < 2 then 
			return nil;
		end
		
		result = stream:ReadByte();
		result = result << 8;
		result = result | stream:ReadByte();
	else 
		result = stream:ReadByte();
	end

	return result;
end

local stream = Stream.Create();
stream:Buffer([[An preost wes on leoden, Laȝamon was ihoten
He wes Leovenaðes sone -- liðe him be Drihten.
He wonede at Ernleȝe at æðelen are chirechen,
Uppen Sevarne staþe, sel þar him þuhte,
Onfest Radestone, þer he bock radde. åäö ÅÄÖ ¢]]);

local c = ReadUTF8(stream);
while c do 
	print(string.format("%x", c),c, utf8char(c));
	c = ReadUTF8(stream);
end

stream:Seek();
print(stream:Read());