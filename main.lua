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

TablePrint(Window);

local p = Process.Open();

local hwnd = Window.Open();

for n=1, #hwnd do 
	if(hwnd[n]:GetIsVisible() and hwnd[n]:GetWindow(4) == nil)then 
		print(hwnd[n]:GetText());
	end
end

hwnd = Window.Open(p:GetID())[1];

local info = hwnd:GetInfo();
print("GetProcessId", hwnd:GetProcessId());
print("GetText", hwnd:GetText());
print("GetIsVisible", hwnd:GetIsVisible());

TablePrint(info);

local function Testwindow()

local c = Window.Create(nil, "class", "Lua", 100 ,100 ,500 ,250);
c:Show(true);

local also = c:CreateButton("Also a button", 0, 50+36, 150, 50,  function(child) 

	local p = child:GetParent();
	print("Also button " .. tostring(p)); 
	p:Redraw();

end);

local textbox = c:CreateTextBox("abc",0,100+36,100,50, true, true, function(child, parent) 

	print(child:GetContent());
end);

local button1 = c:CreateButton("ITS A BUTTON!", 0, 36, 150, 50, function(child)

	print(textbox:GetContent());

	also:Show(not also:GetIsVisible());
	textbox:Enable(not textbox:GetIsEnabled());
	
end);

print(button1:GetContent());

c:SetDrawFunction(function(draw, window)

	local offset = draw:Text(tostring(window));
	draw:SetTextColor(draw:RgbToHex(255, 0, 255));
	draw:SetBackgroundColor(0);
	draw:SetBackgroundMode(2);
	draw:Text("Purple text on black background", 0, offset);
	
	local w,h = draw:GetSize();

	local tw,th = draw:CalcTextSize(w.."x"..h);

	for x=(w/2)-(tw/2)-25, (w/2)+(tw/2)+25 do 
		for y=(h/2)-(th/2)-25, (h/2)+(th/2)+25 do 
			draw:Pixel(x,y, math.random(0, draw:RgbToHex(math.random(0,255),math.random(0,255),math.random(0,255))));
		end
	end

	draw:Text(w.."x"..h, (w/2)-(tw/2), (h/2)-(th/2));
end);

c:Redraw();

local showmsgs = false;
local ok,msgs;
while coroutine.status(c:GetThread()) ~= "dead" do 

	while not c:CheckHasMessage() do 
		Sleep();
	end

	ok,msgs = coroutine.resume(c:GetThread());

	if showmsgs and ok and #msgs > 0 then
		
		print("-----");

		for n=1, #msgs do 
			print(msgs[n].ID, msgs[n].Message, msgs[n].WParam, msgs[n].LParam);
		end
	end
end

end 

Testwindow();

FileSystem.SetCurrentDirectory("C:\\Users\\Terrah\\Desktop");
dofile("recordfiles.lua");
GetKey();
_exit(0);