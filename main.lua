local _exit=Exit;Exit=function(ret) GetKey(); return ret; end
JSON = assert(loadfile "JSON.lua")();
function TablePrint(tbl, depth)

	if(not tbl and depth) then
		assert(tbl, depth);
	end

	depth = depth or 0;

	print(type(depth));

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

print("bla"..c.LF.."bla");

for k,v in pairs(c) do 
	print(k, string.byte(v));
end
print("\n\n");

--TablePrint(Kafka);

SetTitle("librdkafka");

local conf = {};
conf["offset.store.method"]="broker";
conf["enable.partition.eof"]="true";
conf["enable.auto.commit"]="true";
conf["auto.offset.reset"]="earliest";
conf["group.id"]="LUA";

local c = assert(Kafka.NewConsumer(conf));
c:Logs("E:/kafka.log");
c:AddBroker("192.168.2.170");
local ok, err;

local meta, err = c:GetMetadata(1000);
while not meta do 
	print(err);
	meta,err = c:GetMetadata(1000);
end
print("Connected");
DumpToFile("E:/meta.json", meta);
DumpToFile("E:/group.json", c:GetGroups());
local hi,lo;

local topics = {};

for n=1, #meta.Topics do 
	
	if(not meta.Topics[n].Name:match("^__"))then

		io.write("Subscribing to "..tostring(meta.Topics[n].Name).." ");

		ok, lo, hi = c:GetOffsets(meta.Topics[n].Name, 0);

		if ok then

			io.write("["..lo.." "..hi.."] ");

			ok, err = c:Subscribe(meta.Topics[n].Name, 0, lo);

			if ok then 
				print("OK");
				table.insert(topics, ok);
			else 
				print("FAIL: "..err);
			end
		else 
			print("Unable to retrive offsets for "..meta.Topics[n].Name);
		end
	end
end 

print("Owner: "..c:GetId());
local msg;
local data;
while true do 
	
	for n=1, #topics do
		msg = c:Poll(topics[n]);
		if msg then 
			data = msg:GetData();
			print("["..data.Topic.."] ["..data.Error.."] ["..data.Partition..":"..data.Offset.."] ["..msg:GetOwnerId().."]: "..data.Payload);
			--[[if(data.ErrorCode == 0)then 
				ok, err = c:Commit(msg);
				io.write("COMMIT: "..tostring(ok));
				if err then 
					print(" "..err);
				else 
					print(" ");
				end 
			end]]
		end
	end

	if HasKeyDown() and GetKey() == 27 then 
		return;
	end

	Sleep(1);
end 