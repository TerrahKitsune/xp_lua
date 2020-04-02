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

local mutex = Mutex.Open("test");

local aeskey = string.fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
local iv = string.fromhex("000102030405060708090a0b0c0d0e0f");

local aes = Aes.Create(aeskey, iv);

local testdata = "6bc1bee22e409f96e93d7e117393172a";
testdata = testdata:upper();

local encrypted = string.tohex(aes:Encrypt(string.fromhex(testdata)));
aes:SetIV(iv);
local decrypted = string.tohex(aes:Decrypt(string.fromhex(encrypted)));

print(encrypted);
print(testdata);
print(decrypted);

print("---");

aes = Aes.Create(aeskey);

local result = string.tohex(aes:Encrypt(string.fromhex(testdata)));
local decrypted = string.tohex(aes:Decrypt(string.fromhex(result)));

print(result);
print(testdata);
print(decrypted);

print(mutex:Lock());
print(mutex:Info());
GetKey();
print(mutex:Unlock());

local procs = Process.Start(nil,"cmd",nil,false, false);
Console.Attach(procs:GetID());
print = Console.Print;
print(procs:GetID(), procs:GetName());

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