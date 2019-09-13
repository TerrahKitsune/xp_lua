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

print("bla"..c.LF.."bla");

for k,v in pairs(c) do 
	print(k, string.byte(v));
end
print("\n\n");

--TablePrint(Kafka);

local c = assert(Kafka.NewConsumer("test"));
c:Logs("E:/kafka.log");
c:AddBroker("10.9.23.252");
print(1,c);
local meta, err = c:GetGroups();
print(2,c, meta, err);
if meta then 
	print(type(meta));
	TablePrint(meta);
else 
	print(err);
end

--c:CreateTopic("test");
local result = c:Subscribe("test");
print(c);
print(c:Logs());

meta, err = c:GetMetadata(100);
print(c);
while not meta do 
	print(err);
	meta, err = c:GetMetadata(100);
end

print(type(meta));
print(JSON:encode_pretty(meta));
TablePrint(result);
TablePrint(c:Subscribe("TutorialTopic"));
local msg;
while true do 
	msg = c:Poll();
	while msg do 
		TablePrint(msg);
		msg = c:Poll();
	end

	Sleep(100);
end 