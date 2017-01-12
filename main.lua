local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

local processes = {};
local proc = nil;
for k,v in pairs(Process.All()) do 
	proc = Process.Open(k);
	if proc then
		table.insert(processes,proc);
	end
end

while true do

	for k,v in ipairs(processes) do 
		print(v:GetID(), v:GetName(), v:GetCPU(), v:GetRAM());
	end

	Sleep(1000);
	CLS();
end

return 1;