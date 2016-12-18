local _exit=Exit;Exit=function()GetKey();_exit();end

local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

SetTicker(function() print("meow!") end);

print(assert(GetRegistryValue(0,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion","SystemRoot")));
ToggleConsole(false);
print(Runtime());
Sleep(1000);
ToggleConsole(true);
for n=1, 100 do 
	Sleep(10);
	print(Runtime());
end