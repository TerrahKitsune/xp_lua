local _exit=Exit;Exit=function(ret) GetKey(); return ret+1; end

local tlk = TLK.Open("C:\\Games\\Steam\\steamapps\\common\\Neverwinter Nights 2\\dialog.TLK");
local all = tlk:GetAll();

--[[for k,v in pairs(all) do 
	local data = tlk:Get(k);
	
	if data.Flags & 0x0006 > 0 then 
		for kk,vv in pairs(data) do 
			print(kk,vv);
		end 
	end
end]]

function TablePrint(tbl)

	for k,v in pairs(tbl) do 
		print(k,v);
	end

end

local new = TLK.Create("G:\\test.tlk", all);

TablePrint(new:Get(1));
print("---------");
print(new:Set(1, ""));
print("---------");
TablePrint(new:Get(1));
print(new:Set(1, "Pizza"));
print("---------");
TablePrint(new:Get(1));