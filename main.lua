local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

print("hello world");

local http = Http.Start("GET", "https://www.google.se/");

while http:GetStatus() do 
	print("Fetching...");
end 

local code, ok, contents, header = http:GetResult();

print(code, ok, contents);
