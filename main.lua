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

local pipe = Pipe.Create("Test");

local ok = pipe:Available();

if not ok then print(GetLastError()) else print("Ok "..tostring(ok)); end

local client = Pipe.Open("Test");

ok = pipe:Available();
if not ok then print(GetLastError()) else print("Ok "..tostring(ok)); end

ok = client:Available();
if not ok then print(GetLastError()) else print("Ok "..tostring(ok)); end

client:Close();

ok = pipe:Available();
if not ok then print(GetLastError()) else print("Ok "..tostring(ok)); end

ok = client:Available();
if not ok then print(GetLastError()) else print("Ok "..tostring(ok)); end