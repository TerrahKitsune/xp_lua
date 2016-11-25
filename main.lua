local sq = SQLite.Open();
print(sq:Query("CREATE TABLE uuids(Data INT);"));

for n=1,1000000 do
	sq:Query("INSERT INTO uuids VALUES(@data);", {data=UUID()});
end

print(sq:Query("SELECT * FROM uuids;"));
print(" ");
while sq:Fetch() do
	for k,v in pairs(sq:GetRow()) do
		print(k,v);
	end
	print("---");
end

if true then return 1; end

local server = "10.9.23.254";
local user = "lua";
local password = "";
local db = user;
local port = 3306;

local function GetBic()
	local sql = MySQL.Connect(server,user,password,db,port);
	if(sql == nil)then
		error("Unable to connect to database");
	end

	sql:Query("SELECT Data FROM test WHERE Name='Test';");
	if not sql:Fetch() then
		error("Unable to fetch data");
	end

	local gff = sql:GetRow(1);
	return GFF.OpenString(gff);
end

XP = dofile("xp_craft.lua");
if XP then
	print("Tint_Hair|Tintable|Tint|1|r",XP:NWGetValue("Tint_Hair|Tintable|Tint|1|r","test","1"));
	print("Tint_Hair|Tintable|Tint|1|g",XP:NWGetValue("Tint_Hair|Tintable|Tint|1|g","test","1"));
	print("Tint_Hair|Tintable|Tint|1|b",XP:NWGetValue("Tint_Hair|Tintable|Tint|1|b","test","1"));
	print("Tint_Hair|Tintable|Tint|1|a",XP:NWGetValue("Tint_Hair|Tintable|Tint|1|a","test","1"));
	XP:ClearCache();
	print("FirstName",XP:NWSetValue("FirstName","test","1","Leio"));
	XP:ClearCache();
	print("FirstName",XP:NWGetValue("FirstName","test","1"));
	return XP;
end

function Padding(depth)
	local padding = "";
	for n=1,depth do
		padding = padding .. " ";
	end
	return padding;
end

function PrintTable(tbl, depth)
	for k,v in pairs(tbl) do
		if type(v) == "table" then
			print(Padding(depth)..tostring(k),"\t",tostring(v));
			if(log~=nil)then
				log:write(Padding(depth)..tostring(k),tostring(v),"\n");
			end
			PrintTable(v,depth+1);
		else
			if(log~=nil)then
				log:write(Padding(depth)..tostring(k),"\t",v,"\n");
			end
			print(Padding(depth)..tostring(k),v);
		end
	end
end

function test()

	local timer = Timer.New();
	timer:Start();


	local sql = MySQL.Connect(server,user,password,db,port);
	if(sql == nil)then
		print("Unable to connect to database");
	else
		print("Connection OK");

		sql:Query("SELECT Data FROM test WHERE Name='Test';");
		if not sql:Fetch() then
			print("Unable to fetch data");
		end

		local gff = sql:GetRow(1);
		local bic = GFF.OpenString(gff);

		for n=1,#bic.Fields do
			if bic.Fields[n].Label == "FirstName" then
				print("First Name: " .. bic.Fields[n].Data.Strings[1].String);
				break;
			end
		end

		for n=1,#bic.Fields do
			if bic.Fields[n].Label == "Cha" then
				print("Charisma: " .. bic.Fields[n].Data);
				bic.Fields[n].Data = bic.Fields[n].Data + 1;
				break;
			end
		end

		print(sql:Query("UPDATE test SET Data="..sql.EncodeString(GFF.SaveToString(bic)).." WHERE Name='Test';"));
	end
	timer:Stop();
	print("SQL + GFF took "..tostring(timer:Elapsed()).." ms");
	return true;
end



local log = assert(io.open("log.txt", "w"));


local file = "E:/Media/Documents/Neverwinter Nights 2/localvault/leioana2.bic";
--local file = "D:/vhc_store_uniqueweapons.UTM";
local inp = assert(io.open(file, "rb"));
--local gff = GFF.OpenFile(file);
local data = inp:read("*all")
inp:close();
tim = Timer.New();
print(tim:Start());
local gff = GFF.OpenString(data);
tim:Stop();
print(tim:Elapsed());
GFF.SaveToFile(gff,"E:/Media/Documents/Neverwinter Nights 2/localvault/test.bic");
local strgff = GFF.SaveToString(gff);
print(strgff);
--PrintTable(gff,0);
log:close();
print("-----------------------------");
return gff;
