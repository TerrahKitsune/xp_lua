local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

--[[local int = 123;
local packed = string.pack("i", int);
print(packed);
print(string.unpack("i", packed));

if true then return; end]]

local client = {};

client.Client = Client.Connect("localhost", 8001);

local function CreateCoRo(newclient) 

newclient.CO = coroutine.create(function ()

	local cli = newclient;
	local len = nil;
	local result = "";
	local tim = nil;

	local func = function()

		if tim ~= nil and tim:Elapsed() > 2000 then
		
			len = nil;
			tim = nil;
			return nil;
		end

		if not cli.Client:Status() then
			return nil;
		end

		local event = cli.Client:GetEvent();

		local sanity = Timer.New();
		sanity:Start();

		while event do 

			if event.type == 4 then 

				if len == nil then 

					if event.data:len()<5 then
						return nil;
					end 

					local controlbyte, length = string.unpack("Bi",event.data);

					if controlbyte ~= 255 then 

						cli:Reconnect();
						return nil;
					end

					len = length;

					result = event.data:sub(6);

					if result:len() == len then 
						len = nil;
						return result;
					else 
						tim = Timer.New();
						tim:Start();
					end
				else
					result = result .. event.data;

					if result:len() == len then 
						len = nil;
						tim = nil;
						return result;
					elseif result:len() > len then 
						len = nil;
						tim = nil;
						return nil;
					else 
						tim = Timer.New();
						tim:Start();
					end
				end
			end 

			if not cli.Client:Status() then
				return nil;
			elseif sanity:Elapsed() > 25 then
				return nil;
			end

			event = cli.Client:GetEvent();
		end

		return nil;
	end
	
	while true do 
		coroutine.yield(func())
	end
end)

end 

CreateCoRo(client);

function client:Reconnect()
	self.Client:Disconnect();
	self.Client = Client.Connect("localhost", 8001);
end

function client:Send(data)

	self.Client:Send(string.pack("B", 255)..string.pack("i", data:len())..data);
end 

function client:Recv()
	local ok, result = coroutine.resume(self.CO);
	return result;
end

while true do 
	
	local data = client:Recv();
	if data then
		print(data,data:len());
	else 
		client:Send("Hello this is lua!");
	end

	Sleep(1000);
end 

GetKey();

