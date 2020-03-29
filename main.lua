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

math.randomseed(os.time());
math.random(); math.random(); math.random();

local function PrintMsgs(ftp, timeout)
	
	local msgs=ftp:GetMessages(timeout);

	for n=1, #msgs do print(msgs[n]); end

	assert(ftp:GetConnectionStatus());
end

FTP.SetTimeout(5);

local ftp = FTP.Open("10.9.23.250");
assert(ftp:Login("ftp", "meowCat69!"));

local ip, port = ftp:Passive();
ip="10.9.23.250";

assert(ftp:Command("LIST"));
PrintMsgs(ftp, 0);

local channel = assert(FTP.OpenDataChannel(ip, port));
print(channel);
local ok, data = channel:Recv();
while ok do 

	if data:len() > 0 then 
		io.write(data);
	end

	ok, data = channel:Recv();
end

channel:Close();

ip, port = ftp:Passive();
ip="10.9.23.250";

PrintMsgs(ftp);
assert(ftp:Command("STOR bot.sqlite"));
PrintMsgs(ftp, 5);

local f = io.open("E:/bot.sqlite", "rb");

local cnt = 0;
local prev = 0;

channel = assert(FTP.OpenDataChannel(ip, port));
while channel:GetConnectionStatus() and f do 

	data = f:read(100000);

	if data == nil then 
		f:close();
		f = nil;
	else

		cnt = cnt + data:len();
		prev = WriteStatusString("Write: "..tostring(cnt), prev, 250);

		channel:Send(data);
	end
end

WriteStatusString("Write: "..tostring(cnt), prev, 0);
prev = 0;
print(" ");

channel:Close();

PrintMsgs(ftp, 5);

ip, port = ftp:Passive();
ip="10.9.23.250";

PrintMsgs(ftp);

assert(ftp:Command("RETR bot.sqlite"));
f = io.open("R:/bot.sqlite", "wb");

channel = assert(FTP.OpenDataChannel(ip, port));
ok, data = channel:Recv();
cnt = 0;

while ok do 

	if data:len() > 0 then 

		cnt = cnt + data:len();
		prev = WriteStatusString("Read: "..tostring(cnt), prev, 250);

		f:write(data);
		f:flush();
	end

	ok, data = channel:Recv();
end

f:close();

WriteStatusString("Read: "..tostring(cnt), prev, 0);
print(" ");

channel:Close();

ip, port = ftp:Passive();
ip="10.9.23.250";

PrintMsgs(ftp);
assert(ftp:Command("STOR bot2.sqlite"));
PrintMsgs(ftp, 5);

f = io.open("R:/bot.sqlite", "rb");

cnt = 0;
prev = 0;

channel = assert(FTP.OpenDataChannel(ip, port));
while channel:GetConnectionStatus() and f do 

	data = f:read(100000);

	if data == nil then 
		f:close();
		f = nil;
	else

		cnt = cnt + data:len();
		prev = WriteStatusString("Write: "..tostring(cnt), prev, 250);

		channel:Send(data);
	end
end

WriteStatusString("Write: "..tostring(cnt), prev, 0);
print(" ");