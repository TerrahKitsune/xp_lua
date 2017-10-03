local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local db = MySQL.Connect("10.9.23.254", "lua", "KMU4ePmFSYRu3PGS", "lua");

local cnt=0;

while(true)do

print(db:Query("SELECT * FROM weather"));

while db:Fetch() do 
	local row = db:GetRow();
	print(row.irl_location, row.weather_description);
end

cnt = cnt + 1;
print(cnt);

Sleep(1000);
end 
--[[local hak = ERF.Open("E:/Media/Documents/Neverwinter Nights 2/hak/haven_basedata_v1.hak");

local data = {};

for k,v in pairs(hak:GetKeys())do
	print(v.File);
	hak:Extract(k,"e:/test/"..v.File);
	table.insert(data, "e:/test/"..v.File);
end


local test = ERF.Create("e:/test.hak", "HAK\0", data, 2, "bingus");
print(test);

for k,v in pairs(test:GetStrings()) do
	print(v.String);
end

for k,v in pairs(test:GetKeys()) do

	print(v.File);
	test:Extract(v.ResID, "E:/test2/"..v.File);
end]]