local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

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

for n=1, #ARGS do 
	print(n, ARGS[n]);
end

if(ARGS[3]) then 
	SetTitle("Client");

	local pipe = Pipe.Open("test1");

	local test = pipe:Read();
	while test do 
		print(test);
		test = pipe:Read();
	end
	GetKey();
	_exit(7);
else 
	SetTitle("Server");
end 

local pipe = Pipe.Create("test1");

local proc = Process.Start(nil,ARGS[1].." main.lua test",nil,false, false);

local bla, err = pipe:Write("Hi");
print(GetLastError(err));
while err == 536 do 
	bla, err = pipe:Write("Hi");
	print(GetLastError(err));
end
print(bla,err);

print(pipe:Write("Meow"));

local exit = proc:GetExitCode();
while not exit do 
	Sleep(1);
	exit = proc:GetExitCode();
end

print(exit);

return exit;