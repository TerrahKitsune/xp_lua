local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

function TablePrint(tbl)

	print(tostring(tbl));

	if type(tbl)~="table" then 	
		return;
	end

	for k,v in pairs(tbl) do 
		print(k,v);
	end

end

function ArrayPrint(arr)

	print(tostring(arr));

	if type(tbl)~="table" then 
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

local ok, err;
local db = assert(MySQL.Connect("10.9.23.252", "Terrah", , "kitsunebot"));
while true do 

	local test={};

	ok, err = db:Query("SELECT * FROM messages;");

	if ok then 
		
		while db:Fetch() do 
			table.insert(test, db:GetRow());
		end 

		print("Rows: "..tostring(#test));
	else 
		print(err);
	end 
end 