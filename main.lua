local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local server = "10.9.23.254";
local user = "lua";
local password = "Kah9LpSp9UEZA6qf";
local db = user;
local port = 3306;

local server = Server.Start(1234);
local cli;
local ip,port;
local method,data;

print(server);

while server do 
	cli = server:Accept();
	if cli then 
		ip,port = server:GetIP(cli);
		print("Connected",ip,port);
	end 

	for _,i in ipairs(server:Clients()) do 
		method,data = server:Recv(i);
		if method then 
			print(method);
			for k,v in pairs(data) do 
				print(k,v);
			end
		end

		server:Send(i,"Test",{Korv="Sås",Nubr=123.4,IsOk=true});
	end

	Sleep(5);
end 

return 1;