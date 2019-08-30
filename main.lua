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

TablePrint(Services);

local test = Services.All();
assert(test, GetLastError());
TablePrint(#test);

local stream = Stream.Open("E:/burg.bic");
stream:Save("E:/test.bic");
local f =io.open("E:/test.txt", "wb");
f:close();
stream:WriteToFile("E:/test.txt", 20, 8);
stream:Seek();
stream:WriteToFile("E:/test.txt", 0, 8);

stream:Seek();
stream:WriteToFile("E:/test.txt", 8, 8);

stream:Seek();
stream:WriteToFile("E:/test.txt", 1000, 8);

local other = Stream.Create();
other:ReadFromFile("E:/test.txt", 8, 8);
other:Seek();
print(other:Read());