local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

if true then return; end

local srv = Server.Start(8001);

local n=1;

while true do 

	local ev = srv:GetEvent();
	while ev do

		print("Type: "..tostring(ev.type));
		print("Socket: "..tostring(ev.socket));
		print("Data: "..tostring(ev.data));
		ev = srv:GetEvent();
		Sleep(100);
	end

	Sleep(100);

	local clients = srv:GetClients();
	for k,v in pairs(clients) do 
		print(k,v);
		srv:Send(k,tostring(k).." "..v.." "..tostring(n));
		n = n + 1;
	end

	Sleep(10000);
end 

GetKey();

