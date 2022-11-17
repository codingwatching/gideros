Gltf=Core.class()

function Gltf:init(path,name)
	self.path=path
	if name then
		local f=io.open(path.."/"..name)
		self.desc=json.decode(f:read("*a"))
		f:close()
	end
end

function Gltf:getScene(i)
	i=i or self.desc.scene+1
	local ns=self.desc.scenes[i]
	local root={}
	root.type="group"
	root.parts={}
	root.name="s"..i
	root.bones={}
	self.collectBones=root.bones
	for ni,n in ipairs(ns.nodes) do
		root.parts["n"..n]=self:getNode(n+1)
	end
	self.collectBones=nil
	if self.desc.animations then
		local PATHLEN={ scale=3, rotation=4, translation=3 }
		root.animations={}
		for an,ad in ipairs(self.desc.animations) do
			local abones={}
			local btab={}
			for _,c in ipairs(ad.channels) do
				local bone=btab[c.target.node]
				local samp=ad.samplers[c.sampler+1]
				if not bone then
					bone={ boneId="n"..c.target.node, samplerIn=samp.input, keyframes={} }
					btab[c.target.node]=bone
					abones[#abones+1]=bone
					local kfrms=self:getBuffer(samp.input+1,false,1)
					for ki,kv in ipairs(kfrms) do
						bone.keyframes[ki]={ keytime=(kv-kfrms[1])*1000 }
					end
				else
					assert(bone.samplerIn==samp.input)
				end
				local path=c.target.path
				local pl=PATHLEN[path]
				local kfrms=self:getBuffer(samp.output+1,false,pl)
				for ki,kv in ipairs(bone.keyframes) do
					local b=(ki-1)*pl
					local v={}
					for n=1,pl do v[n]=kfrms[n+b] end
					kv[path]=v
				end
			end
			root.animations[an]={ bones=abones, name=ad.name }
		end
	end
	return root
end

function Gltf:getNode(i)
	local nd=self.desc.nodes[i]
	local root={}
	root.type="group"
	root.parts={}
	root.name=nd.name
	if nd.mesh then
		local md=self.desc.meshes[nd.mesh+1]
		local bones
		if nd.skin then
			local sd=self.desc.skins[nd.skin+1]
			bones={}
			local mats=self:getBuffer(sd.inverseBindMatrices+1,false,16)
			for k,v in ipairs(sd.joints) do
				local n=(k-1)*16+1
				local rm=Matrix.new() rm:setMatrix(
					mats[n+0],mats[n+1],mats[n+2],mats[n+3],
					mats[n+4],mats[n+5],mats[n+6],mats[n+7],
					mats[n+8],mats[n+9],mats[n+10],mats[n+11],
					mats[n+12],mats[n+13],mats[n+14],mats[n+15])
				rm:invert()
				bones[k]={ node="n"..v, _poseMat=rm }
				if self.collectBones then self.collectBones["n"..v]=true end
			end
		end
		for pi,prim in ipairs(md.primitives) do
			local function bufferIndex(str)
				for n,id in pairs(prim.attributes) do
					if n:sub(1,#str)==str then return id+1 end
				end
				return 0
			end
			local animi=self:getBuffer(bufferIndex("JOINTS_0"),false,4)
			local animw=self:getBuffer(bufferIndex("WEIGHTS_0"),false,4)
			local m={
				vertices=self:getBuffer(bufferIndex("POSITION"),false,3), 
				texcoords=self:getBuffer(bufferIndex("TEXCOORD")),
				normals=self:getBuffer(bufferIndex("NORMAL")),
				colors=self:getBuffer(bufferIndex("COLOR_0"),false,4), 
				indices=self:getBuffer(prim.indices+1,true),
				animdata=animi and animw and { bi=animi, bw=animw },
				type="mesh",
				material=self:getMaterial((prim.material or -1)+1),
				bones=bones,
			}
			root.parts["p"..pi]=m
		end
	end
	if nd.children then
		for ni,n in ipairs(nd.children) do
			root.parts["n"..n]=self:getNode(n+1)
		end
	end
	if nd.matrix then 
		root.transform=nd.matrix
	else
		root.srt={ s=nd.scale, r=nd.rotation, t=nd.translation }
	end
	return root
end

function Gltf:getBuffer(i,indices,vlen)
	local bd=self.desc.accessors[i]
	if bd==nil then return nil end
	if bd._array then return bd._array end
	local t={}
	local buf,stride=self:getBufferView(bd.bufferView+1)
	local bc=bd.count
	local bm=1
	if bd.type=="SCALAR" then --
	elseif bd.type=="VEC2" then bm=2
	elseif bd.type=="VEC3" then bm=3
	elseif bd.type=="VEC4" then bm=4
	elseif bd.type=="MAT4" then bm=16
	else assert(false,"Unhandled type:"..bd.type)
	end
--	local tm=os:clock()
--	print("ACC:",i,buf:size(),bd.count,bc,bd.componentType)
	--[[
	GL_BYTE (5120)
	GL_DOUBLE (5130)
	GL_FALSE (0)
	GL_FLOAT (5126)
	GL_HALF_NV (5121)
	GL_INT (5124)
	GL_SHORT (5122)
	GL_TRUE (1)
	GL_UNSIGNED_BYTE (5121)
	GL_UNSIGNED_INT (5125)
	GL_UNSIGNED_INT64_AMD (35778)
	GL_UNSIGNED_SHORT (5123)
	]]
	local cl,dv=0,""
	if bd.componentType==5126 then
		cl=4
		dv="f"
	elseif bd.componentType==5123 then
		cl=2
		dv=if bd.normalized then "S" else "s"
	elseif bd.componentType==5121 then
		cl=1
		dv=if bd.normalized then "B" else "b"
	elseif bd.componentType==5125 then
		cl=4
		dv=if bd.normalized then "I" else "i"
	else
		assert(false,"Unhandled componentType:"..bd.componentType)
	end
	if stride>0 then stride=stride-cl*bm end
	local br=bd.byteOffset or 0
--	print(br,bm,bc,stride)
	local ii=1
	for ci=1,bc do
		for mi=1,bm do
			if bd.componentType==5126 then
				t[ii]=buf:get(br,4):decodeValue("f")
				br+=4
			elseif bd.componentType==5123 then
				t[ii]=buf:get(br,2):decodeValue(dv)
				if indices then 
					t[ii]+=1 
				elseif bd.normalized then
					t[ii]/=65535
				end
				br+=2
			elseif bd.componentType==5121 then
				t[ii]=buf:get(br,1):decodeValue(dv)
				if indices then 
					t[ii]+=1 
				elseif bd.normalized then
					t[ii]/=255
				end
				br+=1
--[[NVHalf				
			elseif bd.componentType==5121 then
				local n=buf:get(br,2):decodeValue("S")
				local e=(n>>10)&0x1F
				local m=(n&0x3FF)/1024
				local v=if e==0 then m/16384 else (1+m)*(2^(e-15))
				if n&0x8000 then v=-v end
				t[ii]=v
				if indices then 
					t[ii]+=1 
				elseif bd.normalized then
					t[ii]/=65535
				end
				br+=2 ]]
			elseif bd.componentType==5125 then
				t[ii]=buf:get(br,4):decodeValue(dv)
				if indices then t[ii]+=1 end
				br+=4
			else
				assert(false,"Unhandled componentType:"..bd.componentType)
			end
			ii+=1
		end
		if vlen==4 and bm==3 then
			t[ii]=1 ii+=1
		elseif vlen==3 and bm==4 then
			t[ii]=nil
			ii-=1
		end
		br+=stride
	end
	--[[
	if (os:clock()-tm)>.1 then
		print(i,json.encode(bd)," in ",os:clock()-tm)
	end
	]]
	bd._array=t
	return t
end

local gltfNum=0
function Gltf:getBufferView(n,ext)
	local bd=self.desc.bufferViews[n]
	local buf=self.desc.buffers[bd.buffer+1]
	if not buf.data then -- gideros buffer
		if buf.uri then
			local d=buf.uri:sub(1,37)
			if d=="data:application/octet-stream;base64," then
				buf.data=Cryptography.unb64(buf.uri:sub(38))
			end
		end
		if not buf.data then buf.data=self:loadBuffer(bd.buffer+1,buf) end -- gideros buffer
	end
	gltfNum+=1
	local bname="_gltf_"..gltfNum..(ext or "")
	local bb=Buffer.new(bname) -- embedded texture buffer
	bb:set(buf.data:sub((bd.byteOffset or 0)+1,(bd.byteOffset or 0)+bd.byteLength))
	return bb,bd.byteStride or 0,bname
end

function Gltf:getImage(n)
	local bd=self.desc.images[n]
	if not bd then return nil end
	if bd.uri then
		return self.path.."/"..bd.uri
	end
	if bd.bufferView then
		local iext=nil
		if bd.mimeType=="image/jpeg" then iext=".jpg"
		elseif bd.mimeType=="image/png" then iext=".png"
		end
		assert(iext,"Unsupported image type:"..bd.mimeType)
		local _,_,bname=self:getBufferView(bd.bufferView+1,iext)
		return "|B|"..bname -- |B| Gideros embedded texture buffer
	end
end

function Gltf:loadBuffer(i,buf)
	local f=io.open(self.path.."/"..buf.uri)
	local data=f:read("*a")
	f:close()
	return data
end

function Gltf:getMaterial(i)
	local bd=self.desc.materials[i]
	if bd==nil then return nil end
	if bd._mat then return bd._mat end
	local mat={}
	if bd.pbrMetallicRoughness then
		mat.kd=bd.pbrMetallicRoughness.baseColorFactor
		if mat.kd then
			for i=1,4 do mat.kd[i]=mat.kd[i]^.3 end
		end
		local td=bd.pbrMetallicRoughness.baseColorTexture -- the embedded image texture data
		if td and td.index then
			local embedded=self:getImage(td.index+1) -- embedded = Gideros Buffer holding the image texture data
			mat.embeddedtexture = Texture.new(embedded,true,{ wrap=TextureBase.REPEAT, extend=false})
		end
		-- TO DO NORMAL MAP TEXTURE?
	end
	bd._mat=mat
	return mat
end

-- ***********************************************************
Glb=Core.class(Gltf,function (path,name) return path,nil end)

function Glb:init(path,name)
	local fn=name
	if path then fn=path.."/"..name end
	local f=io.open(fn)
	assert(f,"File not found:"..fn)
	self.binData=f:read("*a")
	f:close()

	local hdr=self.binData:decodeValue("iii")
	assert(hdr[1]==0x46546c67,"Not a glb file"..name)
	local length=hdr[3]-12
	local l=13

	local chunks={}
	while length>=8 do
		local chdr=self.binData:sub(l,l+7):decodeValue("ii")
		local cl,_ct=chdr[1],chdr[2]
--		print("CHUNK",("%08x:%08x"):format(cl,ct))
		table.insert(chunks,{type=chdr[2],length=chdr[1],start=l+8})
		l+=8+cl
		length-=(8+cl)
	end
	assert(chunks[1].type==0x4E4F534A,"GLB: first buffer should be JSON")
	self.binChunks=chunks
	self.desc=json.decode(self.binData:sub(chunks[1].start,chunks[1].start+chunks[1].length-1))
--	print(json.encode(self.desc))
end

function Glb:loadBuffer(i,buf)
	return self.binData:sub(self.binChunks[i+1].start,self.binChunks[i+1].start+self.binChunks[i+1].length-1)
end
