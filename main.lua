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

print("hi");

local proc = Process.Start(nil,"echo hello world!",FileSystem.CurrentDirectory(),true, true);

print(proc:ReadFromPipe());