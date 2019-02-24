local _exit=Exit;Exit=function(ret) GetKey(); return ret; end

require "NeuralNetwork"

function TablePrint(tbl)

	print(tostring(tbl));

	if type(tbl)~="table" then 	
		return;
	end

	for k,v in pairs(tbl) do 
		print(k,v);
	end

end

function ArrayPrint(arr)

	print(tostring(arr));

	if type(tbl)~="table" then 
		return;
	end

	for n=1,#arr do 
		print(arr[n]);
	end 
end

for n=1, #ARGS do 
	print(n, ARGS[n]);
end

function PrintPixel(px)
	io.write("{"..px.r.." | ");
	io.write(px.g .. " | ");
	print(px.b .. "} ");
end

local t = Image.Load("E:/c.bmp");
local matrix = t:GetPixelMatrix();

PrintPixel(matrix[1][1]);
PrintPixel(matrix[1][100]);
PrintPixel(matrix[100][1]);
PrintPixel(matrix[100][100]);

--[[for y=1, #matrix do 
	for x=1, #matrix[y] do 
		io.write(y.." "..x..": ");
		PrintPixel(matrix[y][x]);
	end
end]]

t:SetPixelMatrix(matrix);
t:Save("E:/d.bmp");

local test = Image.Screenshot(0,0,0,0, 2);
local t = test:Crop(100,100,1820,980);
t:Save("e:/crop.bmp");

print(test);
local pixels = test:GetPixels();

print(pixels);

for n=1, #pixels do 
	local lum = .2126 * pixels[n].r + .7152 * pixels[n].g + .0722 * pixels[n].b;
	
	--[[io.write(n..": {");
	io.write(pixels[n].r.." | ");
	io.write(pixels[n].g .. " | ");
	io.write(pixels[n].b .. "} ");
	print(lum);]]

	if lum > 127.5 then
		pixels[n].r = 255;
		pixels[n].g = 255;
		pixels[n].b = 255;
	else
		pixels[n].r = 0;
		pixels[n].g = 0;
		pixels[n].b = 0;
	end
end

test:SetPixels(pixels);
test:Save("E:/test.bmp");

function CreateTest(name, func) 

	local img = Image.Create(1920,1080);

	local pixels = img:GetPixels();

	for n=1,#pixels do 

		pixels[n].r = func(n);
		pixels[n].g = func(n);
		pixels[n].b = func(n);
	end 

	img:SetPixels(pixels);

	img:Save(name);
end 

print(test:GetSize());

CreateTest("E:/random.bmp", function() return math.random(255); end );
CreateTest("E:/random20.bmp", function() return 255/math.random(20); end );
local last=-1;
local r;
CreateTest("E:/random2.bmp", function(n) 
	 
	if n == last then 
		
	else 
		last = n; 
		r = math.random(2); 
	end
	if r == 1 then 
		return 255; 
	else 
		return 0; 
	end 
end );