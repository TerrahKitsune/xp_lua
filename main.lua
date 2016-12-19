local _exit=Exit;Exit=function()GetKey();_exit();end

local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

local sq = assert(SQLite.Open("D:\\Media\\Desktop\\uuid\\jpk.db"));
assert(sq:Query("SELECT Name FROM `articles` WHERE ID=15;"));
if sq:Fetch() then 
	local name = sq:GetRow(1); 
	print(name);
	print(MySQL.EncodeString(name));
end 