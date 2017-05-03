local _exit=Exit;Exit=function(ret)print(ret); GetKey(); return ret+1; end

local code, ok, contents, header = Http.Request("GET", "https://pastebin.com/raw/1fbiWK1D", nil, nil);

print(code, ok, contents, header);
