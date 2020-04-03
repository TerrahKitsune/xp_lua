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

local listener = assert(Socket.Listen(112));
local client, data;

while true do 

	client = listener:Accept();

	if client then

		print(client:Info());

		while client:HasData() do 
			data = client:Read();
			io.write(data);
		end

		client:Close();
	else
		Sleep();
	end
end 