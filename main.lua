local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

local twoda = TWODA.Open("E:/Media/Desktop/Checkbuild/2da/skills.2da");
assert(twoda,"Unable to load "..tostring(v));
local columns = twoda:GetInfo();
for i,v in ipairs(columns) do 
	print(i,v);
end 