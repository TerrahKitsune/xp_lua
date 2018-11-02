local _exit=Exit;Exit=function(ret) GetKey(); return ret+1; end


function TablePrint(tbl)

	for k,v in pairs(tbl) do 
		print(k,v);
	end

end

local tlk = TLK.Open("G:\\test.tlk");

--tlk:Set(1, "Nerd");
tlk:Defragment(1000);
TablePrint(tlk:Get(1));

if true then return; end

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


local new = TLK.Create("G:\\test.tlk", all);


print(UUID());