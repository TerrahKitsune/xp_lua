local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

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

local gigabuttonoffset = 275;

local function CreateGCPrint()
	SetGCFunction({last=Time()}, function(obj) local t=Time();print("COLLECTING GARBAGE Lua mem: "..math.floor(collectgarbage("count")) .. " KB Time: "..(t-obj.last)); CreateGCPrint(); end);
end
CreateGCPrint();
collectgarbage();

local gigabutton;
local w, co = Window.Create(nil, "lua", "Lua Window", 500, 500, 500, 500, 0xcf0000, function(event)

	if gigabutton then 
		local info = gigabutton:Size();
		gigabutton:Move(0, gigabuttonoffset, event.Width, event.Height - gigabuttonoffset);
	end

end);
local text = w:CreateTextBox("", 0, 50, 200, 25);

local combo = w:CreateComboBox(200,0,100,100, function(comboBoxWindow, parentWindow, data)
	text:SetContent(data);
end);

local button = w:CreateButton("Button!", 0,0,100, 50, 
function(buttonWindow, parentWindow) 
	print(buttonWindow:GetContent(), parentWindow:GetContent())
	buttonWindow:Destroy();
end);

local button = w:CreateButton("Quit!", 100,0,100, 50, 
function(buttonWindow, parentWindow) 
	parentWindow:Destroy();
end);

local listbox = w:CreateListBox(200, 75, 200, 200, function(comboBoxWindow, parentWindow, data)
	print(comboBoxWindow, parentWindow, data);
	text:SetContent(data);
end);

local listview = w:CreateListView(0, 75, 200, 200, {"Row", "Test", "Hidden"}, function(comboBoxWindow, parentWindow, data)
	print(comboBoxWindow, parentWindow, data);
	text:SetContent(data);
end);

listview:SetListViewColumnWidth(1, 100);
listview:SetListViewColumnWidth(2, 100);
listview:SetListViewColumnWidth(3, 0);

local info = w:GetInfo();

local progress = w:CreateProgressbar(0, gigabuttonoffset, info.Client.Width, 50);

gigabuttonoffset = gigabuttonoffset + 50;

print(0,gigabuttonoffset, info.Client.Width, info.Client.Height - gigabuttonoffset);

local static = w:CreateStaticText("Static text", 0, gigabuttonoffset, info.Client.Width, 50, 1);

gigabuttonoffset = gigabuttonoffset + 50;

gigabutton = w:CreateButton("GIGA BUTTON!", 0,gigabuttonoffset, info.Client.Width, info.Client.Height - gigabuttonoffset, 
function(buttonWindow, parentWindow) 

	local number = tonumber(text:GetContent():ToAnsi());

	if not number then

		listview:SetListViewText(4,1, text:GetContent());
		print(listview:GetBoxSelectedIndex());
		print(listbox:GetBoxSelectedIndex());
		print(combo:GetBoxSelectedIndex());

		listview:SetBoxSelectedIndex();
		listbox:SetBoxSelectedIndex();
		combo:SetBoxSelectedIndex();
	else 

		static:SetContent(progress:GetProgressbarStep());

		if number ~= 0 then
			progress:StepProgressbar(number);
		end
	end
end);

listview:AddBoxItem({1, "Hi", 1});
listview:AddBoxItem({2, "Hello", 2});
listview:AddBoxItem({3, "Cats", 3});
listview:AddBoxItem({444, "asd", 4});
listview:AddBoxItem({5, "123", 5});

listview:RemoveBoxItem(1);
local items = listview:GetBoxItems(1);
local items2 = listview:GetBoxItems(2);

for i=1,#items do
	print(items[i], items2[i]);
end

for n=1, 10 do
	listbox:AddBoxItem("Test "..n);
end

listbox:RemoveBoxItem(2);

items = listbox:GetBoxItems();

for i=1,#items do
	print(items[i]);
end

combo:AddBoxItem("Test 1");
combo:AddBoxItem("Test 2");
combo:AddBoxItem("Test 3");
combo:RemoveBoxItem(2);

items = combo:GetBoxItems();

for i=1,#items do
	print(items[i]);
end

w:Show(true);

local ok, msgs;
while coroutine.status(co) ~= "dead" do 

	while w:CheckHasMessage() do
		ok, msgs = coroutine.resume(co);

		for n=1, #msgs do 
			--print(msgs[n].Message);
		end
	end 

	Sleep();
end

_exit(0);