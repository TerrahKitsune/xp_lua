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

local u, r = UUID();
local t = string.format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", string.byte(r,1,16));
print(u, r);
print(t, r);

for n=1, r:len() do 
	print(r:byte(n));
end

local stream = Stream.Create();
stream:Write("abc123");
print("-------------------");
stream:Seek();
for n=1, stream:len() do 
	print(stream:pos(), string.char(stream:ReadByte()), stream:len());
	stream:Shrink();
end
print("-------------------");
stream:Seek();
stream:Buffer("Meowcat 1000");
for n=1, stream:len() do 
	print(stream:pos(), string.char(stream:ReadByte()), stream:len());
end

for n=1, 100000 do 
	stream:Buffer("Meowcat 1000");
end

print(stream:Read());