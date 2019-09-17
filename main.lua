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

local addr="192.168.2.170";

local c = assert(Kafka.NewConsumer(conf));
c:Logs("E:/kafka.log");
c:AddBroker(addr);
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

			local commit = c:GetCommitedOffset(meta.Topics[n].Name, 0, 1000);

			io.write("["..lo.." "..hi.."] ["..commit.."] ");

			ok, err = c:Subscribe(meta.Topics[n].Name, 0, hi);

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
ok = nil;
for n=1, #topics do 
	if topics[n]:GetInfo() == "short" then 
		ok = n;
		break;
	end
end

print("short ->", ok);

if ok then 
	table.remove(topics, ok):Dispose();
	--c:DeleteTopic("short");
end 

--assert(c:CreateTopic("short", 1));
assert(c:AlterConfig(2, "short", "cleanup.policy", "delete"));
assert(c:AlterConfig(2, "short", "retention.ms", "3000"));
assert(c:SetPartitions("short", 5));

conf = c:GetConfig(4, "0");
print("CONF:-----");
for k,v in pairs(conf) do 
	print(k, v);
end

conf = c:GetConfig(2, "temp");
print("CONF:-----");
for k,v in pairs(conf) do 
	print(k, v);
end

conf = {};
ok, err = c:Subscribe("short", 0, 0, conf);
assert(ok, err);
table.insert(topics, ok);

ok, err = c:Subscribe("short", 1, 0, conf);
assert(ok, err);
table.insert(topics, ok);

print(c:PauseTopic(ok));

local p = assert(Kafka.NewProducer(conf));
assert(p:AddBroker(addr));
local ptopic = assert(p:Subscribe("temp", 0, 0, conf));
print(c:ResumeTopic(ok));

ok, err = p:GetGroups();
while not ok do 
	print(err);
	ok, err = p:GetGroups();
end

assert(p:Send(ptopic, "Hello!"));

print("Owner: "..c:GetId());
local msg;
local data;
local time,ty;
local cnt=0;
local tim = Timer.New();
tim:Start();
while true do 
	
	msg = c:Events() or p:Events();
	--[[data = UUID();
	cnt = cnt + 1;

	if tim:Elapsed() > 1000 then 
		ok, err = p:Send(ptopic, data, 0, tostring(cnt));

		if not ok then
			print("Send error: "..err);
		end
		tim:Stop();
		tim:Reset();
		tim:Start();
	end]]

	while msg do 
		print("EVENT---");
		for k,v in pairs(msg) do 
			print(k, v);
		end 
		print("--------");
		msg = c:Events() or p:Events();
	end

	for n=1, #topics do
		msg = c:Poll(topics[n]);
		if msg then 
			data = msg:GetData();
			time, ty = msg:GetTimestamp();
			print("["..data.Topic.."] ["..data.Error.."] ["..data.Partition..":"..data.Offset.."] ["..msg:GetOwnerId().."] ["..ty..":"..time.."] ["..msg:GetLatency().."] "..tostring(data.Key)..": "..data.Payload);
			if(data.ErrorCode == 0)then 
				
				--[[ok, err = c:Commit(msg);
				ok, err = c:CommitOffsets(data.Topic, data.Partition, data.Offset);
				io.write("COMMIT: "..tostring(ok));
				if err then 
					print(" "..err);
				else 
					print(" ");
				end ]]
			end
		end
	end

	if HasKeyDown() and GetKey() == 27 then 
		return;
	end

	Sleep(1);
end 