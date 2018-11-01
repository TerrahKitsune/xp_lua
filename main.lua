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

local new = TLK.Create("G:\\test.tlk", all);
local newall = new:GetAll();
local cnt = tlk:GetInfo();

for k,v in pairs(all) do 
	local data = tlk:Get(k);
	
	if data.Flags & 0x0006 > 0 then 
		if data.Flags & 0x0004 then
			new:SetSoundInfo(k, data.SoundResRef, data.SoundLength);
		else 
			new:SetSoundInfo(k, data.SoundResRef);
		end
	end
end

for n=0,cnt do 
	if all[n] ~= newall[n] then 
		print(n, "|"..all[n].."|", "|"..newall[n].."|");
	end 
end

print(tlk:GetInfo());
print(new:GetInfo());
