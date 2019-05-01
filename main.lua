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

print(UUID());

local buffer = Stream.Create();

for n=1, 2000000 do 
	buffer:WriteByte(n%255);
end 

print(buffer:GetInfo());
buffer:Seek();

local n=1;
local b = buffer:ReadByte();
while b ~= -1 do 
	
	if b ~= n%255 then 
		print(b, n%255);
		error("Fail "..tostring(n));
	end
	n = n + 1;
	b = buffer:ReadByte();
end
print(buffer:GetInfo());

buffer:Seek(1000);
buffer:WriteByte(255);
buffer:Seek(1000);
print(buffer:ReadByte(), 255);

buffer:Seek(2000);

for n=1, 2000000 do 
	buffer:WriteByte(n%255);
end 

print(buffer:GetInfo());

buffer:Seek(2000);

n=1;
b = buffer:ReadByte();
while b ~= -1 do 
	
	if b ~= n%255 then 
		print(b, n%255);
		error("Fail "..tostring(n));
	end
	n = n + 1;
	b = buffer:ReadByte();
end

print(buffer:GetInfo());

buffer:Seek();
local other = Stream.Create();
print(other:Write("abc"));
print(other:Write("123"));
print(other:Write(math.pi));
print(other:Write(true));
print(other:Write(buffer, 5000));

other:Seek();
local test = other:Read(3);
print(test);
test = other:Read(3);
print(test);

other:Seek();
other:WriteFloat(123.45);
other:Seek();
print(other:ReadFloat());

other:Seek();
other:WriteDouble(54.321);
other:Seek();
print(other:ReadDouble());

other:Seek();
other:WriteShort(111);
other:Seek();
print(other:ReadShort());

other:Seek();
other:WriteUnsignedShort(222);
other:Seek();
print(other:ReadUnsignedShort());

other:Seek();
other:WriteInt(333);
other:Seek();
print(other:ReadInt());

other:Seek();
other:WriteUnsignedInt(444);
other:Seek();
print(other:ReadUnsignedInt());

other:Seek();
other:WriteLong(555);
other:Seek();
print(other:ReadLong());

other:Seek();
other:WriteUnsignedLong(666);
other:Seek();
print(other:ReadUnsignedLong());

buffer:Close();