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
ll:AddFirst("Hi");
ll:AddFirst("There!");
ll:AddFirst(123);
ll:AddLast(1.578);
ll:AddFirst(print);
ll:AddFirst({});
ll:AddFirst(ll);
print(ll);

ll:Insert(1, "This is FIRST");
ll:Insert(ll:Count(), "This is LAST");
ll:Insert(100, "This is DEAD");

ll:AddFirst("FIRST");
ll:AddLast("LAST");

ll:Insert(5, "This is 5");

for i,v in ll:Forward() do 
	print(i,v);
end 
print("-----");
for i,v in ll:Backward() do 
	print(i,v);
end 
print("-----");
print(ll:Get(5));
print(ll:Count());
print(ll:Get(1));
print(ll:Get(7));
print("-----");
for n=1, ll:Count() do 
	print(n, ll:Get(n));
end 

print(ll:Remove(6));
print(ll:Remove(1));
print(ll:Remove(ll:Count()));
print("-----");
for i,v in ll:Forward() do 
	print(i,v);
end 
print("-----");
for i,v in ll:Backward() do 
	print(i,v);
end 
print("----- del");
for i,v in ll:Forward() do 
	print(ll:Remove(i));
end 

print(ll:Count());
print("----- gdsgs");
for i,v in ll:Forward() do 
	print(i,v);
end 