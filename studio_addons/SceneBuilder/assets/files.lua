Files={}

function Files.loadJson(file)
	local js=Files.load(file)
	if not js then return end
	if js:sub(1,1)=="x" then js=zlib.decompress(js) end
	local ok,ret=pcall(json.decode,js)
	if not ok then return end
	return ret
end

function Files.saveJson(file,data,compress)
	local jdata=json.encode(data)
	if compress then jdata=zlib.compress(jdata) end
	return Files.save(file,jdata)
end

function Files.load(file)
	local fd=io.open(file,"rb")
	if not fd then return end
	local js=fd:read("*all")
	fd:close()
	return js
end

function Files.save(file,data)
	local fd=io.open(file,"wb")
	if not fd then return false end
	fd:write(data)
	fd:close()
	return true
end
