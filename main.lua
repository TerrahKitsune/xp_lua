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

local proc=Process.Open();
print(proc:Affinity());

local threads = proc:Threads();

for n=1, #threads do 
	print("-----");
	for k,v in pairs(threads[n]) do 
		print(k,v);
	end
end