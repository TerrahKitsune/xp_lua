local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

function TablePrint(tbl, depth)

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

print("bla"..c.LF.."bla");

for k,v in pairs(c) do 
	print(k, string.byte(v));
end
print("\n\n");
ArrayPrint(ODBC.GetAllDrivers());
print("\n\n");

TablePrint(ODBC);
local odbc = assert(ODBC.DriverConnect("DRIVER={PostgreSQL ODBC Driver(ANSI)};Server=192.168.1.207;Port=5432;Database=test;Uid=ancon;Pwd=ancon;"));
TablePrint(odbc);

print("\n\n");


assert(odbc:Prepare('select COUNT(*) as cnt from public.pizza;'));
assert(odbc:Execute());
assert(odbc:Fetch(), "No data");
local count = odbc:GetRow().cnt;
print("Count: "..count);

math.randomseed(os.time())
math.random(); math.random(); math.random()

local id, burk = UUID();
local testid = id;

assert(odbc:Begin());
assert(odbc:Prepare([[INSERT INTO public.pizza("Id", "Radius", "Thickness", "Data", "Name", "GuidId")	VALUES (?, ?, ?, ?, ?, ?);]]));
for n=1,40000 do
	
	count = count + 1;
	assert(odbc:Bind(count));
	assert(odbc:Bind(math.random()));
	assert(odbc:Bind(math.floor(math.random()*100)));
	assert(odbc:Bind(burk, true));
	assert(odbc:Bind("Pizza "..id.." "..(count+1)));
	assert(odbc:Bind(id));
	assert(odbc:Execute());
	id = UUID();
end

assert(odbc:Commit());

assert(odbc:Prepare('select * from public.pizza WHERE "Id"=? OR "GuidId"=?;'));
assert(odbc:Bind(count-1));
assert(odbc:Bind(testid));
assert(odbc:Execute());
while odbc:Fetch() do 

	TablePrint(odbc:GetRowColumnTypes());
	TablePrint(odbc:GetRow());

end 

assert(odbc:Prepare('select * from public.pizza;'));
assert(odbc:Execute());
while odbc:Fetch() do 
	print(odbc:GetRow().GuidId);
end 

odbc:Disconnect();