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

print("bla"..c.LF.."bla");

for k,v in pairs(c) do 
	print(k, string.byte(v));
end
print("\n\n");

--TablePrint(Kafka);

SetTitle("librdkafka");

local autocommit = true;
local conf = {};
conf["offset.store.method"]="broker";
--conf["enable.partition.eof"]="true";
conf["enable.auto.commit"]=tostring(autocommit);
conf["auto.offset.reset"]="earliest";
conf["group.id"]="LUA";
print(conf["enable.auto.commit"]);
local c = assert(Kafka.NewConsumer(conf));
c:Logs("E:/kafka.log");

c:AddBroker("10.9.23.248");

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

		for i=1, #meta.Topics[n].Partitions do

			io.write("Subscribing to "..tostring(meta.Topics[n].Name).."."..meta.Topics[n].Partitions[i].Id.." ");

			ok, lo, hi = c:GetOffsets(meta.Topics[n].Name, meta.Topics[n].Partitions[i].Id);

			if ok then

				io.write("["..lo.." "..hi.."] ");

				ok, err = c:Subscribe(meta.Topics[n].Name, meta.Topics[n].Partitions[i].Id, lo);

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
end 

conf = assert(c:GetConfig(2, "temp"));

if conf["cleanup.policy"] ~= "delete" then
	assert(c:AlterConfig(2, "temp", "cleanup.policy", "delete"));
end 

if conf["retention.bytes"] ~= "1073741824" then
	assert(c:AlterConfig(2, "temp", "retention.bytes", "1073741824"));
end 

if conf["retention.ms"] ~= "60000" then
	assert(c:AlterConfig(2, "temp", "retention.ms", "60000"));
end 

DumpToFile("E:/topicconf.json", c:GetConfig(2, "temp"));

print("Owner: "..c:GetId());
local msg;
local data;
local time,ty;
while true do 
	
	ok=false;
	msg = c:Events();

	while msg do 
		print("EVENT---");
		for k,v in pairs(msg) do 
			print(k, v);
		end 
		print("--------");
		msg = c:Events();
	end

	for n=1, #topics do
		msg = c:Poll(topics[n]);
		if msg then 
			ok = true;
			data = msg:GetData();
			time, ty = msg:GetTimestamp();
			print("["..data.Topic.."] ["..data.Error.."] ["..data.Partition..":"..data.Offset.."] ["..msg:GetOwnerId().."] ["..ty..":"..time.."] ["..msg:GetLatency().."] "..tostring(data.Key)..": "..data.Payload);
			if not autocommit and data.ErrorCode == 0 then 
				ok, err = c:Commit(msg);
				if ok then 
					print("Commit: OK");
				else 
					print("Commit: FAIL "..err);
				end
			end
		end
	end

	if HasKeyDown() and GetKey() == 27 then 
		return;
	end

	if not ok then
		Sleep(1);
	end
end 