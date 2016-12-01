local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

local sql = MySQL.Connect(server,user,password,db,port);
print(sql);	

local tim = Timer.New();
print(tim);