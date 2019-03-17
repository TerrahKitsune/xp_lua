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

local img = Image.Screenshot();
local data = img:GetPixelMatrix();
img:Close();
img=nil;