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

local sha = SHA256.New();
sha:Update("Hi");
print(sha:Finish());

if true then return; end 

--TablePrint(Kafka);

SetTitle("librdkafka");

local delete=false;
local subscribe = false;
local autocommit = false;
local conf = {};
conf["offset.store.method"]="broker";
conf["enable.partition.eof"]="false";
conf["enable.auto.commit"]=tostring(autocommit);
conf["auto.offset.reset"]="earliest";
conf["group.id"]="LUA";
conf["client.id"]="LUA";
print(conf["enable.auto.commit"]);
local c = assert(Kafka.NewConsumer(conf));
c:Logs("E:/kafka.log");
local addr = "192.168.2.170";
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
local hashttp = false;

local function SubscribeAll(c)

	local meta = assert(c:GetMetadata(10000));
	local ok, hi,lo,co;
	local topics = {};
	local err;

	for n=1, #meta.Topics do 
	
		if(not meta.Topics[n].Name:match("^__"))then

			for i=1, #meta.Topics[n].Partitions do

				io.write("Subscribing to "..tostring(meta.Topics[n].Name).."."..meta.Topics[n].Partitions[i].Id.." ");

				ok, lo, hi = c:GetOffsets(meta.Topics[n].Name, meta.Topics[n].Partitions[i].Id);

				co, err = c:GetCommitted(meta.Topics[n].Name, meta.Topics[n].Partitions[i].Id);

				if not co then 
					co = err;
				end 

				if ok then

					io.write("["..lo.." "..hi.." "..co.."] ");

					if meta.Topics[n].Name == "HttpRequests" then 
						hashttp = true;
						lo = hi;
					end 

					ok, err = c:OpenTopic(meta.Topics[n].Name, meta.Topics[n].Partitions[i].Id, hi);

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

	return topics;
end

if subscribe then

	local ok, msg, data, time, ty;

	local test = "temp";

	print(c:GetOffsets(test, 0));
	print(c:GetCommitted(test, 0));

	assert(c:Subscribe(test));

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

		msg = c:Poll();
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

		if HasKeyDown() and GetKey() == 27 then 
			return;
		end

		if not ok then
			Sleep(1);
		end
	end

else

	local topics = SubscribeAll(c);

	local HttpTest = nil;
if hashttp then

	local producer = Kafka.NewProducer();
	producer:AddBroker(addr);
	local ptopic = producer:OpenTopic("HttpResponses");

HttpTest = function(msg)
	print("http");
	local data = JSON:decode(msg.Payload);
	TablePrint(data);

	local resp = {StatusCode=200, RedirectUri="", Headers={}};
	
	resp.Headers["Content-Type"] = "application/json";
	resp.Body = msg.Payload;

	producer:Send(ptopic, JSON:encode_pretty(resp), nil, msg.Key);
end

	end

	local find = table.select(topics, function(k,v) if v:GetInfo()=="temp" then return k; end end);

	if delete and #find>0 then 
		print("Deleting topic temp");
		ok = assert(c:DeleteTopic("temp"));
		assert(ok.ErrorCode==0, ok.Error);
		find={};
	end

	if #find <= 0 then 
		print("Creating topic temp");
		ok = assert(c:CreateTopic("temp"));
		assert(ok.ErrorCode==0, ok.Error);
	end

	if #find < 10 then 
		print("Creating topic paritions");
		ok = assert(c:SetPartitions("temp", 10));
		assert(ok.ErrorCode==0, ok.Error);
	end

	for n=1, #topics do topics[n]:Dispose(); end
	topics = SubscribeAll(c);

	conf = assert(c:GetConfig(2, "temp"));
	while #table.select(conf, function(k, v) return k; end) <= 0 do 
		conf = assert(c:GetConfig(2, "temp"));
	end

	if conf["cleanup.policy"] ~= "delete" then
		print("cleanup.policy", "delete");
		ok = assert(c:AlterConfig(2, "temp", "cleanup.policy", "delete"));
		for k,v in pairs(ok) do print(k,v); end
	end 

	if conf["retention.bytes"] ~= "1073741824" then
		print("retention.bytes", "1073741824");
		ok = assert(c:AlterConfig(2, "temp", "retention.bytes", "1073741824"));
		for k,v in pairs(ok) do print(k,v); end
	end 

	if conf["retention.ms"] ~= "5000" then
		print("retention.ms", "5000");
		ok = assert(c:AlterConfig(2, "temp", "retention.ms", "5000"));
		for k,v in pairs(ok) do print(k,v); end
	end 

	DumpToFile("E:/topicconf.json", c:GetConfig(2, "temp"));

	print("Owner: "..c:GetId());
	print("Partitions: "..#topics);
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
			msg = c:Consume(topics[n]);
			if msg then 
				ok = true;
				data = msg:GetData();
				time, ty = msg:GetTimestamp();

				if HttpTest and data.Topic == "HttpRequests" then 					
					print(pcall(HttpTest,data));
				end

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
end