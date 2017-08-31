local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local cli = Client.Connect("localhost", 8001);

while true do 
	
	local status = cli:Status();

	while status == false do
		print("Not connected");
		Sleep(100);
		status = cli:Status();
	end 

	cli:Send("Hi!");

	while status do 

		local ev = cli:GetEvent();
		while ev do 
			print("Type: "..tostring(ev.type));
			print("Socket: "..tostring(ev.socket));
			print("Data: "..tostring(ev.data));
			hadevent = true;
			
			lastevent = ev;
			Sleep(100);

			ev = cli:GetEvent();
		end 

		status = cli:Status();
	end 
	Sleep();
end 

GetKey();