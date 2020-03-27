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

math.randomseed(os.time());
math.random(); math.random(); math.random();

local function PrintMsgs(msgs)

	for n=1, #msgs do print(msgs[n]); end
end

setenv("Hello", "Test", true);
print(getenv("Hello"));

if true then return; end 

TablePrint(FTP);

FTP.SetTimeout(5);

local ftp, welcomemsg = assert(FTP.Open("comput"));

for n=1, #welcomemsg do 
	print(welcomemsg[n]);
end

local result, err = assert(ftp:Login("admin", "hej123"));

local ip, port = assert(ftp:Passive());
print(ip, port);

msgs, err = assert(ftp:Command("LIST"));
TablePrint(msgs);

print(ftp:OpenDataChannel(ip, port, function(recv) io.write(recv); return true; end));

ip, port = assert(ftp:Passive());

msgs, err = assert(ftp:Command("STOR Ancon.vhd"));
TablePrint(msgs);

local f = FileAsync.Open("C:/Ancon.vhd", "rb");
f:Read();

print(ftp:OpenDataChannel(ip, port, function(recv) 

	local current, max = f:BufferStatus();
	local data, ok = f:EmptyBuffer();
	
	PrintMsgs(ftp:GetMessages());

	SetTitle(tostring(max).."/"..tostring(current));

	return ok, data; 
end));

PrintMsgs(ftp:GetMessages());

f:Close();