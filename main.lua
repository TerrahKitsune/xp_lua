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

local ll = LinkedList.New();

local key = ll:AddFirst("FIRST");
ll:AddBefore(key, "BEFORE 1");
ll:AddAfter(key, "AFTER 1");
ll:AddBefore(key, "BEFORE 2");
ll:AddAfter(key, "AFTER 2");

local index= ll:IndexFromKey(key);
print("Key", key);
print("Index", index);
print("Data from key", ll:GetFromKey(key));
print("Data from index", ll:GetFromIndex(index));

index = 1;
key = ll:KeyFromIndex(index);
print("index key",index, key);
print("Data from key", ll:GetFromKey(key));
print("Data from index", ll:GetFromIndex(index));

index = ll:Count();
key = ll:KeyFromIndex(index);
print("index key",index, key);
print("Data from key", ll:GetFromKey(key));
print("Data from index", ll:GetFromIndex(index));

for i,v, id in ll:Forward() do 
	print(i, v, id);
	if ll:IndexFromKey(id) == i and ll:KeyFromIndex(i) == id and v == ll:GetFromKey(id) and v == ll:GetFromIndex(i) then 
		print("OK");
	else 
		print("BAD");
	end
end 
print("---");
for i,v, id in ll:Backward() do 
	print(i,v, id);
	if ll:IndexFromKey(id) == i and ll:KeyFromIndex(i) == id and v == ll:GetFromKey(id) and v == ll:GetFromIndex(i) then 
		print("OK");
	else 
		print("BAD");
	end
end 

while true do 
	ll:AddFirst(UUID());
end 
