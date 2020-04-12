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
local last=Time();
local function CreateGCPrint()
	SetGCFunction({}, function() local t=Time();print("COLLECTING GARBAGE Lua mem: "..math.floor(collectgarbage("count")) .. " KB Time: "..(t-last)); last=t; CreateGCPrint(); end);
end
CreateGCPrint();
collectgarbage();

local garbage = {t=true, f=false, n=0/0, pinf=math.huge,ninf=-math.huge, mem={}, pi=math.pi, func=print}

for n=1, 7 do 
	table.insert(garbage.mem, GlobalMemoryStatus(n));
end

garbage.Recurse = garbage;

local arr = {}
for n=1, 7 do 
	table.insert(arr,GlobalMemoryStatus(n));
end

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

local encodedTest = Stream.Create();
local function EncoderFunc(text)
	encodedTest:Buffer(text);
end

local decodedTest = Stream.Create();
local function DecoderFunc()
	local t = decodedTest:Read(100);
	return t;
end

local encodetable = Base64.GetEncodeTable();
encodetable = encodetable:gsub("/", "-");
Base64.SetEncodeTable(encodetable);

text = "abc 123 hej 123 meow Meow \0\0asd";
data = Base64.Encode(text);
print(text);
print(data);
print(Base64.Decode(data));
print(Base64.Decode(data) == text);

Break();

collectgarbage();
collectgarbage();
collectgarbage();

local testdata = {};
local db=assert(MySQL.Connect("10.9.23.252", "TwitchKafka", "meowCat69!", "twitch"));
assert(db:Query("SELECT * FROM messages LIMIT 1000;"));
while db:Fetch() do 
	table.insert(testdata, db:GetRow());
end

local text, data; 

local j=Json.Create();
local t=Timer.New();
while true do 

	t:Start();
	text = j:Encode(testdata);
	data = j:Decode(text);

	text = j:EncodeToFile("r:/test.json", data);
	data = j:DecodeFromFile("r:/test.json", text);

	encodedTest:Shrink();
	j:EncodeToFunction(EncoderFunc, data);

	decodedTest:Shrink();
	decodedTest:Buffer(encodedTest:Read());
	data = j:DecodeFromFunction(DecoderFunc);

	CheckIsEqual(testdata, data);
	CheckIsEqual(data, testdata);

	t:Stop();
	t:Reset();
	t:Start();
	print("Cycle time: "..tostring(t:Elapsed()));
	Sleep();

	if HasKeyDown() and GetKey() == string.byte(' ') then 
		break;
	end
end 
text = nil;
data = nil;
t= nil;
db=nil;
testdata=nil;
collectgarbage();
collectgarbage();
collectgarbage();
Sleep(1000);
collectgarbage();

--FileSystem.SetCurrentDirectory("C:\\Users\\Terrah\\Desktop\\TwitchToKafkaAdminer");
--dofile("C:\\Users\\Terrah\\Desktop\\TwitchToKafkaAdminer\\main.lua");