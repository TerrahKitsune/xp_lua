local function CopyTo(from, to)

	if type(from)~="table" or #from == 0 then 
		return;
	end 

	for n=1, #from do 
		table.insert(to, from[n]);
	end 
end 

local function GetLast(msgs)

	if type(msgs)~="table" or #msgs == 0 then 
		return 0, "0 Timeout";
	end 

	local last = msgs[#msgs];
	local code = last:match("^(.-)%s");

	code = tonumber(code);
	
	if code == nil then 
		return 0, "0 Bad server response";
	end 

	return code, last;
end

return function(addr, port, user, password, timeout, endl)

endl = endl or "\n";
timeout = timeout or 5;
FTP.SetTimeout(timeout);

local ftp = {Addr=addr, Port=port, User=user, Password=password, Msgs={}, Timeout = timeout, Endl = endl};

function ftp:Connect()

	self:Close();

	local ok, err = FTP.Open(self.Addr, self.Port);

	if ok then 
		self.FTP = ok;
		for n=1, #err do 
			table.insert(self.Msgs, err[n]);
		end 
	else 
		return false, err;
	end

	self.FTP:SetEndline(self.Endl);

	ok, err = self.FTP:Login(self.User, self.Password);

	if not ok then 
		self:Close();
		return false, err;
	else 
		CopyTo(self.FTP:GetMessages(), self.Msgs);
	end

	return true;
end 

function ftp:Close()
	if self.FTP then 
		self.FTP:Close();
		self.FTP=nil;
	end
end

function ftp:GetMessages() 

	local current = self.Msgs;
	self.Msgs={};
	return current;
end

function ftp:Delete(file)

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("DELE "..file);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if code == 250 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:DirectoryCreate(dir)

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("MKD "..dir);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if code == 257 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:Rename(from, to)

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("RNFR "..from);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs); 
	
	if code ~= 350 then 
		return false, msg;
	end 

	ok, err = self.FTP:Command("RNTO "..to);

	if not ok then 
		return false, err;
	end 
	
	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs); 
	
	if code == 250 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:DirectoryDelete(dir)

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("RMD "..dir);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs); 
	
	if code == 250 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:DirectoryUp(dir)

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("CWD "..dir);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if code == 250 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:DirectoryDown()

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("CDUP");

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if code == 250 or code == 200 then 
		return true;
	else 
		return false, msg;
	end 
end

function ftp:Upload(remotefile, localfile, progressfunc)

	local fileinfo = FileSystem.GetFileInfo(localfile);
	
	if not fileinfo then 
		return false, "File does not exist "..tostring(localfile);
	end 
	
	local file = io.open(localfile, "rb");
	if not file then 
		return false, "Unable to open file "..tostring(localfile);
	end 

	local size = fileinfo.Size;
	
	CopyTo(self.FTP:GetMessages(), self.Msgs);
	
	local ip, port = self.FTP:Passive();

	if not ip then 
		return false, port;
	elseif self.PassiveOverride then 
		ip = self.PassiveOverride;
	end 

	CopyTo(self.FTP:GetMessages(), self.Msgs);
	
	local ok, err = self.FTP:Command("STOR "..remotefile);
	
	if not ok then 
		file:close();
		return false, err;
	end 
	
	local channel, err = FTP.OpenDataChannel(ip, port);
	
	if not channel then 
		file:close();
		return false, err;
	end 

	local numbmsgs = #self.Msgs;
	local total = 0;
	local data;
	
	local ok = channel:GetConnectionStatus();
	while ok do 
		
		data = file:read(1500);
		
		if not data then 
			break;
		else
			total = total + data:len();
		end 
		
		if not channel:Send(data) then 
			file:close();
			channel:Close();
			return false, "Disconnected";
		end
		
		if progressfunc then 
			ok = progressfunc(size, total);
			
			if not ok then 
				file:close();
				channel:Close();
				return false, "Cancelled";
			end 
		end 

		ok = channel:GetConnectionStatus();
	end

	channel:Close();
	file:close();

	if not ok or size > total then 
		return false, "Disconnected";
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if numbmsgs == #self.Msgs or code ~= 226 then 
	
		numbmsgs = #self.Msgs;
		
		CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);
		
		if numbmsgs == #self.Msgs then 
			return false, "Timeout";
		end
	end

	code, msg = GetLast(self.Msgs);

	if code ~= 226 then 
		return false, msg;
	end

	return true;
end

function ftp:Download(remotefile, localfile, progressfunc)

	CopyTo(self.FTP:GetMessages(), self.Msgs);
	
	local ok, err = self.FTP:Command("SIZE "..remotefile);

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);
	
	local code, msg = GetLast(self.Msgs);
	local size;
	
	if code == 213 then 
	
		size = tonumber(msg:match("%s+(.+)$"));
		
		if not size then 
			return false, "Unable to retreive filesize";
		end
	else 
		return false, msg;
	end
	
	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ip, port = self.FTP:Passive();

	if not ip then 
		return false, port;
	elseif self.PassiveOverride then 
		ip = self.PassiveOverride;
	end 

	CopyTo(self.FTP:GetMessages(), self.Msgs);
	
	local temp = localfile .. ".tmp";
	local file = io.open(temp, "wb");
	
	if not file then 
		return false, "Unable to open local temp file: "..temp;
	end
	
	local ok, err = self.FTP:Command("RETR "..remotefile);
	
	if not ok then 
		file:close();
		FileSystem.Delete(temp);
		return false, err;
	end 

	local channel, err = FTP.OpenDataChannel(ip, port);
	
	if not channel then 
		file:close();
		FileSystem.Delete(temp);
		return false, err;
	end 

	local numbmsgs = #self.Msgs;
	local total = 0;
	local ok, data = channel:Recv();
	while ok do 
		
		total = total + data:len();
		
		if progressfunc then 
			ok = progressfunc(size, total);
			
			if not ok then 
				channel:Close();
				file:close();
				FileSystem.Delete(temp);
				return false, "Cancelled";
			end 
		end 

		file:write(data);
		file:flush();
		
		ok, data = channel:Recv();
	end

	channel:Close();
	file:flush();
	file:close();

	local fileinfo = FileSystem.GetFileInfo(temp);

	if not fileinfo or size > fileinfo.Size then 
		FileSystem.Delete(temp);
		return false, "File transfer failed";
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);

	local code, msg = GetLast(self.Msgs);

	if numbmsgs == #self.Msgs or code ~= 226 then 
	
		numbmsgs = #self.Msgs;
		
		CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);
		
		if numbmsgs == #self.Msgs then 
			FileSystem.Delete(temp);
			return false, "Timeout";
		end
	end

	code, msg = GetLast(self.Msgs);

	if code ~= 226 then 
		FileSystem.Delete(temp);
		return false, msg; 
	end

	FileSystem.Delete(localfile);
	FileSystem.Rename(temp, localfile);
	
	return true;
end

function ftp:DirectoryContents()

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ip, port = self.FTP:Passive();

	if not ip then 
		return false, port;
	elseif self.PassiveOverride then 
		ip = self.PassiveOverride;
	end 

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local ok, err = self.FTP:Command("LIST");

	if not ok then 
		return nil, err;
	end 

	local channel, err = FTP.OpenDataChannel(ip, port);
	
	if not channel then 
		return nil, err;
	end 

	local numbmsgs = #self.Msgs;
	local total = "";
	local ok, data = channel:Recv();
	while ok do 
		total = total .. data;
		ok, data = channel:Recv();
	end

	channel:Close();

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local code, msg = GetLast(self.Msgs);
	
	if numbmsgs == #self.Msgs or code == 150 then 
		numbmsgs = CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);
		if numbmsgs == #self.Msgs then 
			return nil, "Timeout";
		end
	end

	code, msg = GetLast(self.Msgs);
	local t = os.clock();
	while code~=226 do
		
		CopyTo(self.FTP:GetMessages(), self.Msgs);
		
		if os.clock() - t >= self.Timeout then 
			return nil, "Timeout";
		end
		
		code, msg = GetLast(self.Msgs);
	end
	
	if code == 226 then 
	
		local files = {};
		local file;

		for folder, attribs, numb, user, group, size, month, day, tm, name in total:gmatch("(.)(.-)%s+(.-)%s+(.-)%s+(.-)%s+(.-)%s+(.-)%s+(.-)%s+(.-)%s+(.-)\n") do
	
			file={};
			
			file.IsFolder = (folder == 'd' or folder == 'D');
			file.Number = numb;
			file.User = user;
			file.Group = group;
			file.Size = tonumber(size);
			file.Day = tonumber(day);
			
			if name:match("\r$") then	
				file.Name = name:match("(.+)\r$");
			else 
				file.Name = name;
			end
			
			local r,w,x,rr,ww,xx,rrr,www,xxx = attribs:match("(.)(.)(.)(.)(.)(.)(.)(.)(.)");
			
			file.CanRead = r == "r" or r == "R" or rr == "r" or rr == "R" or rrr == "r" or rrr == "R";
			file.CanWrite = w == "w" or w == "W" or ww == "w" or ww == "W" or www == "w" or www == "W";
			file.CanExecute = x == "x" or x == "X" or xx == "x" or xx == "X" or xxx == "x" or xxx == "X";
			
			local monthcmp = month:lower();
			
			if monthcmp == "jan" then
				file.Month = 1;
			elseif monthcmp == "feb" then
				file.Month = 2;
			elseif monthcmp == "mar" then
				file.Month = 3;
			elseif monthcmp == "apr" then
				file.Month = 4;
			elseif monthcmp == "may" then
				file.Month = 5;
			elseif monthcmp == "jun" then
				file.Month = 6;
			elseif monthcmp == "jul" then
				file.Month = 7;
			elseif monthcmp == "aug" then
				file.Month = 8;
			elseif monthcmp == "sep" then
				file.Month = 9;
			elseif monthcmp == "oct" then
				file.Month = 10;
			elseif monthcmp == "nov" then
				file.Month = 11;
			elseif monthcmp == "dec" then
				file.Month = 12;
			else 
				file.Month = month;
			end
			
			hour, mins = tm:match("(..):(..)");

			file.Hour = tonumber(hour);
			file.Min = tonumber(mins);
				
			if file.Hour and file.Min then
				file.Hour = tonumber(hour);
				file.Min = tonumber(mins);
				file.Year = tonumber(os.date("%Y"));
			else
				file.Hour = 0;
				file.Min = 0;
				file.Year = tonumber(tm);
			end
			
			table.insert(files, file);	
		end
	
		return files;
	else 
		return nil, msg;
	end 
end 

function ftp:Ping()

	CopyTo(self.FTP:GetMessages(), self.Msgs);

	local t=Timer.New();
	t:Start();

	local ok, err = self.FTP:Command("NOOP");

	if not ok then 
		return false, err;
	end 

	CopyTo(self.FTP:GetMessages(self.Timeout), self.Msgs);
	t:Stop();
	
	local code, msg = GetLast(self.Msgs);

	if code == 200 then 	
		return true, t:Elapsed();
	else 
		return false, msg;
	end 
end

return ftp;
end