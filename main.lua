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

ArrayPrint(ODBC.GetAllDrivers());

TablePrint(ODBC);
local odbc = assert(ODBC.DriverConnect("DRIVER={PostgreSQL ODBC Driver(ANSI)};Server=192.168.1.207;Port=5432;Database=bleh_core;Uid=ancon;Pwd=ancon;"));
TablePrint(odbc);

assert(odbc:Prepare('SELECT * FROM public."Tenant" WHERE "Name"=?;'));
assert(odbc:Bind("KFC"));
assert(odbc:Execute());
while odbc:Fetch() do 

	TablePrint(odbc:GetRowColumnTypes());
	TablePrint(odbc:GetRow());

end 

odbc:Disconnect();