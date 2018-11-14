local _exit=Exit;Exit=function(ret) GetKey(); return ret+1; end

function TablePrint(tbl)

	for k,v in pairs(tbl) do 
		print(k,v);
	end

end

function ArrayPrint(arr)

	for n=1,#arr do 
		print(arr[n]);
	end 
end

print(Pipe);
TablePrint(Pipe);

local pipe = assert(Pipe.Create("test"));
local remote = assert(Pipe.Open("test"));

print(pipe:Write("Hello this is pipe\n"));
print(pipe:Write("this is also a pipe"));

local data = remote:Read();
while data do 
	print(data);
	data = remote:Read();
end
