if GFFTYPES==nil then
	UNUSED_GFF_ID = 0xFFFFFFFF;
	GFFTYPES = {};
	--type id | actual type | lua type
	GFFTYPES[0] = "BYTE"; --integer
	GFFTYPES[1] = "CHAR"; --integer
	GFFTYPES[2] = "WORD"; --integer
	GFFTYPES[3] = "SHORT";--integer
	GFFTYPES[4] = "DWORD";--integer
	GFFTYPES[5] = "INT";--integer
	GFFTYPES[6] = "DWORD64";--integer
	GFFTYPES[7] = "INT64";--integer
	GFFTYPES[8] = "FLOAT"; --number
	GFFTYPES[9] = "DOUBLE"; --number
	GFFTYPES[10] = "CExoString"; --string
	GFFTYPES[11] = "ResRef"; --string
	GFFTYPES[12] = "CExoLocString"; --table
	GFFTYPES[13] = "VOID"; --string
	GFFTYPES[14] = "Struct"; --struct
	GFFTYPES[15] = "List"; --array of structs
end
--[[
struct:

	*4 chars "BIC", "UTI" etc, only exists on toplevel struct
	string FileType

	*4 chars describing the version usually "V3.2" topstruct only
	string FileVersion

	*programmer defined int of the struct type topstruct is always 0xFFFFFFFF
	int Type

	*Number of fields loaded in Field
	*Largly irrelevant as its recalculated when the gff is saved
	int FieldCount

	*1 index array of all fields in the array
	array Fields

	*The gff type for this table, this can be "topstruct" or "struct"
	string gff

field:

	*Int that represents the type in the field (see GFFTYPES)
	int Type

	*Name of this field (should not be longer then 16 chars)
	string Label

	*Contains the data as speificed by the Type (see GFFTYPES)
	variable Data

	*The gff type for this table, its always "field" on a field
	string gff

CExoLocString:

	*Index into the tlk files, this is 0xFFFFFFFF it unused
	int StringRef

	*The amount of strings this CExoLocString contains
	*Largly irrelevant as its recalculated when the gff is saved
	int StringCount

	*Total size in bytes (minus this field) that the CExoLocString contains
	*Largly irrelevant as its recalculated when the gff is saved
	int TotalSize

	*An array that contains the CExoLocSubstrings
	*this will be empty rather then nil if there are none
	array Strings

CExoLocStringSubString:

	*This is equal to LanguageID * 2 + gender (0 for masculine 1 for feminine)
	int StringID

	*Length of the string
	*Largly irrelevant as its recalculated when the gff is saved
	int StringLength

	*The string this exolocsubstring contains
	string String
]]

--The roadmap filter '|'
local XP = {roadmapfilter='([^|]+)', Cache={}, CacheSize=0, CacheMax=5,mysqlini='E:/Steam/steamapps/common/Neverwinter Nights 2/xp_mysql.ini'};

function XP:ClearCache()
	self.CacheSize=0;
	self.Cache={};
end

function XP:NWExists(roadmap, key, act, value)

	local gff = XP:GetGffFromDB(key, act);

	if gff == nil then
		error("Unable to retrive gff by key "..tostring(key).." from database");
	end

	local field = self:RoadMap(gff,roadmap);
	return field ~= nil;
end

function XP:NWSetValue(roadmap, key, act, value, noupdate)

	local gff = XP:GetGffFromDB(key, act);

	if gff == nil then
		error("Unable to retrive gff by key "..tostring(key).." from database");
	end

	local field = self:RoadMap(gff,roadmap);
	if field == nil then
		error("Unable to retrive field at "..roadmap);
	end

	if field.Type >= 14 then
		error("Field in "..roadmap.." is a struct or list");
	elseif field.Type >= 0 and field.Type <= 9 then

		if type(value) ~= "number" then
			value = tonumber(value);
			if value == nil then
				error("NWSetValue value was not the correct type for field with type "..GFFTYPES[field.Type]);
			end
		end
		field.Data = tonumber(value);
	elseif field.Type == 12 then
		local exo = field.Data;
		value = tostring(value);
		if exo.Strings[1] == nil then
			table.insert(exo.Strings,{StringID = 0, StringLength=value:len()});
		end
		exo.Strings[1].String = value;
	else
		field.Data = tostring(value);
	end

	if not noupdate then
		self:UpdateGffOnDB(key,act,gff);
	end
end

function XP:NWGetValue(roadmap, key, act)

	local gff = XP:GetGffFromDB(key, act);

	if gff == nil then
		error("Unable to retrive gff by key "..tostring(key).." from database");
	end

	local field = self:RoadMap(gff,roadmap);
	if field == nil then
		error("Unable to retrive field at "..roadmap);
	end

	if field.gff == nil or field.gff ~= "field" or field.Type >= 14 then
		error(roadmap.." does not end with a datafield");
	--ExoLocString
	elseif field.Type == 12 then
		local str = field.Data.Strings[1];
		if str == nil or str.String == nil then
			return "";
		else
			return str.String;
		end
	else
		return field.Data;
	end
end

function XP:UpdateGffOnDB(key, act, gff)

	local sql = MySQL.Connect(self.SERVER,self.USER,self.PASSWORD,self.SCHEMA);
	if(sql == nil)then
		error("Unable to connect to database");
	end

	local encoded = sql.EncodeString(GFF.SaveToString(gff));
	act = sql.EncodeString(tostring(act));
	key = sql.EncodeString(tostring(key));
	local ok, msg = sql:Query("INSERT INTO nwnx_tailorstorage (name,act,item)VALUES("..key..","..act..","..encoded..") ON DUPLICATE KEY UPDATE item=VALUES(item);");
	if not ok then
		error(msg);
	end
end

function XP:Uncache(key,act)
	local cachekey = tostring(key).."|"..tostring(act);
	self.Cache[cachekey] = nil;
end

function XP:GetGffFromDB(key,act)

	--Cache
	local cachekey = tostring(key).."|"..tostring(act);
	local gff = self.Cache[cachekey];
	if gff then
		gff.Time = os.clock();
		return gff.Data;
	end

	local sql = MySQL.Connect(self.SERVER,self.USER,self.PASSWORD,self.SCHEMA);
	if(sql == nil)then
		error("Unable to connect to database");
	end

	sql:Query("SELECT item FROM nwnx_tailorstorage WHERE name="..sql.EncodeString(key).." AND act="..sql.EncodeString(act));
	if not sql:Fetch() then
		error("Unable to fetch data");
	end

	local gff = GFF.OpenString(sql:GetRow(1));

	local tick = os.clock();
	local lowest = tick;
	local find = nil;

	--Cache
	while self.CacheSize >= self.CacheMax and self.CacheSize > 0 do

		for k,v in pairs(self.Cache) do
			if v.Time < lowest then
				lowest = v.Time;
				find = k;
			end
		end

		if find then
			self.Cache[find]=nil;
			self.CacheSize = self.CacheSize - 1;
		end
	end

	if self.CacheMax > 0 then
		self.Cache[cachekey] = {Time=tick,Data=gff};
		self.CacheSize = self.CacheSize + 1;
	end

	return gff;
end

function XP:FieldByLabel(struct, path, index)
	if path[index] == nil then
		return struct;
	elseif struct.gff == "struct" or struct.gff == "topstruct" then
		for n=1,#struct.Fields do
			if struct.Fields[n].Label == path[index] then
				return self:FieldByLabel(struct.Fields[n],path,index+1);
			end
		end
		return nil;
	elseif struct.gff == "field" then
		if struct.Type == 14 then
			return self:FieldByLabel(struct.Data,path,index);
		elseif struct.Type == 15 then
			for k,v in pairs(struct.Data) do
				if tostring(k) == path[index] then
					return self:FieldByLabel(v,path,index+1);
				end
			end
		else
			return nil;
		end
	else
		return nil;
	end
end

function XP:RoadMap(gff,roadmap)

	local path = {};

	for word in string.gmatch(roadmap, self.roadmapfilter) do
		table.insert(path,word);
	end

	return self:FieldByLabel(gff,path, 1);
end

--Retrive DB credentials!

local f = io.open(XP.mysqlini, "r");
if not f then
	error("Unable to open "..XP.mysqlini.." to retrive database credentials");
end

local ini = f:read("*all");
f:close();

for k,v in string.gmatch(ini, "([^&=]+)=([^&\n]+)") do
	if(k:find("server")) then
		XP.SERVER = v:gsub("^%s*", "");
	elseif(k:find("user")) then
		XP.USER = v:gsub("^%s*", "");
	elseif(k:find("password")) then
		XP.PASSWORD = v:gsub("^%s*", "");
	elseif(k:find("schema")) then
		XP.SCHEMA = v:gsub("^%s*", "");
	end
end

if XP.SERVER == nil or XP.SERVER == "" then
	error("Unable to retrive mysql server address from "..XP.mysqlini);
elseif XP.USER == nil or XP.USER == "" then
	error("Unable to retrive mysql user from "..XP.mysqlini);
elseif XP.PASSWORD == nil or XP.PASSWORD == "" then
	error("Unable to retrive mysql password from "..XP.mysqlini);
elseif XP.SCHEMA == nil or XP.SCHEMA == "" then
	error("Unable to retrive mysql schema from "..XP.mysqlini);
end

return XP;
