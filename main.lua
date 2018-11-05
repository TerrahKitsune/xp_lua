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

local dirs = FileSystem.GetFiles("D:/Pictures");
local files = FileSystem.GetDirectories("D:/Pictures");

ArrayPrint(dirs);
print("-----------");
ArrayPrint(files);