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

local prefix = "";
local function feedback(total, downloaded) 
	
	if downloaded >= total then
		prvlen = WriteStatusString(prefix..": "..total.."/"..downloaded, prvlen, 0); 
	else 
		prvlen = WriteStatusString(prefix..": "..total.."/"..downloaded, prvlen, 250); 
	end
	return true;
end

local function downloadfiletree(ftp)

	local folder = {};

	local files = assert(ftp:DirectoryContents());
	for n=1, #files do 

		if files[n].IsFolder then 
			local ok, err = ftp:DirectoryUp(files[n].Name);
			assert(ok, tostring(files[n].Name)..": "..tostring(err));
			folder[files[n].Name] = downloadfiletree(ftp);
			assert(ftp:DirectoryDown());
		else
			folder[files[n].Name] = files[n].Size;
		end
	end

	return folder;
end

local name = GetComputerName();
local p = Dns(name, true);

for n=1, #p do 
	for k,v in pairs(p[n]) do
		print(k,v);
	end
end 

print(name..": "..Dns(name));

local ftp = dofile("ftp.lua")("10.9.23.250", 10709, "ftp", "meowCat69!");
ftp.PassiveOverride = "10.9.23.250";

assert(ftp:Connect());

local files = downloadfiletree(ftp);

TablePrint(files);

ArrayPrint(ftp:GetMessages());

ftp:Close();