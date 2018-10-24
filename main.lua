local _exit=Exit;Exit=function(ret) GetKey(); return ret+1; end

print(FileSystem.GetTempFileName(true));
print(FileSystem.GetTempFileName(false));
print(FileSystem.GetTempFileName());

for n=1,#ARGS do 
	print(ARGS[n]);
end

local tmp = io.tmpfile();
tmp:write("abc");
print(tmp);
print(LAST_TEMP_FILE);
GetKey();