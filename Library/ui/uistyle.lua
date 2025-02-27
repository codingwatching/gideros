--!NEEDS:uiinit.lua
--!NEEDS:uiborder.lua
--!NEEDS:uicolor.lua

if _PRINTER then print("uistyle.lua") end

local debug = nil
if debug then print("UI.Style","debug !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") end

UI.Default=UI.Default or {}

local function computeScales()
	local targetTiny		=4.5
	local targetTab			=8
	local targetMonitor		=12

	local dpi,ldpi=application:getScreenDensity()
	dpi=ldpi or dpi or 120 --326 --96

	local dh=application:getDeviceHeight()
	if Oculus then dh=dh/2 end
	local diag=(dh^2+application:getDeviceWidth()^2)^0.5/dpi
	local scw,sch=application:get("screenSize")
	local sdiag=diag
	if scw and sch then sdiag=((scw^2+sch^2)^.5)/dpi end
	
	local ls=application:getLogicalScaleX()
	local tgtdpi=120 --96
	local detectedMode="monitor"
	local platform=application:getDeviceInfo()
	local DesktopPlatforms={
		WinRT=true,
		MacOS=true,
		Linux=true,
	}
	if sdiag>=targetMonitor or DesktopPlatforms[platform] 	then tgtdpi=120 detectedMode="monitor"
	elseif sdiag>=targetTab	then tgtdpi=180 detectedMode="tablet" --240
	elseif sdiag>=targetTiny	then tgtdpi=240 detectedMode="phone" --360
	else tgtdpi=360 detectedMode="tiny" --360
	end
	
	if platform=="iPad" then tgtdpi=180 detectedMode="tablet" end --240 --fix 04/10/2024 iPadMini4
	
	tgtdpi=UI.Default.TargetDpi or tgtdpi

	local zoom=(dpi/tgtdpi)/ls
	local fzoom=20
	if debug then print("UI.Style","getDeviceHeight()",application:getDeviceHeight(),"getDeviceWidth()",application:getDeviceWidth(),"getScreenDensity()",application:getScreenDensity()) end
	if debug then print("UI.Style","windiag",diag,"scrdiag",sdiag,"ls",ls,"dpi",dpi,"tgtdpi",tgtdpi,"zoom",zoom,fzoom,"platform",platform,"mode",detectedMode) end
	return {
		--Screen and window properties
		windowDiagonal=diag,
		screenDpi=dpi,
		screenDiagonal=sdiag,
		--Computed scales and zooms
		logicalScale=ls,
		zoomFactor=zoom,
		zoomTarget=fzoom,
		--Mode detection
		targetTiny=targetTiny,
		targetTab=targetTab,
		targetMonitor=targetMonitor,
		targetDpi=tgtdpi,	
		detectedMode=detectedMode,
	}
end

local screenProperties=computeScales()

UI.Style={}
setmetatable(UI.Style,{
	--[[
	__index=function(t,n)
		if n=="__Reference" then return end
		assert(type(n)=="string","Style index must be a string, not "..type(n))
		assert(not n:find("%."),"Styles can't contain dots")
		if n:lower()==n then 
			t[n]={}
			return t[n] 
		end
	end,]]
	__newindex=function(t,n,v) --Unroll value if a table and if key is all lowercase
		assert(type(n)=="string","Style index must be a string, not "..type(n))
		if type(v)=="table" and not v.__classname and n:lower()==n and not n:find("%.") and not (n:sub(1,1)=="_") then 
			for k,v in pairs(v) do
				t[n.."."..k]=v
			end
		else
			rawset(t,n,v)
		end
	end,
})
UI.Style.__index=UI.Style
UI.Style._style={}
UI.Style._style.__index=UI.Style._style
setmetatable(UI.Style._style,UI.Style)

function UI.Style.getScreenProperties(current) 
	if current then return computeScales() end
	return screenProperties 
end

function UI.Style.updateScreenProperties() 
	screenProperties=computeScales()
	UI.Style.zoomFactor=screenProperties.zoomFactor
	UI.Style.fontSize=(UI.Default.fontSize or screenProperties.zoomTarget*screenProperties.zoomFactor)*(UI.Default.fontScale or 1)
	stage:setStyle(nil,true)
	if UI.Control then
		UI.Control.DRAG_THRESHOLD=stage:resolveStyle("szDragThreshold",UI.Style._style)
		UI.Control.WHEEL_DISTANCE=stage:resolveStyle("szWheelDistance",UI.Style._style)
	end
end

stage:addEventListener(Event.APPLICATION_RESIZE,function ()
	local dpi,ldpi=application:getScreenDensity()
	dpi=ldpi or dpi
	if dpi and screenProperties.screenDpi~=dpi then
		UI.Style.updateScreenProperties()
	end	
end)

function UI.Style:setDefault(style)
	--Unlink current style from root style
	local m=self._style
	while true do
		local mm=getmetatable(m)
		if not mm then break end
		if mm==UI.Style then
			setmetatable(m,nil)
			break
		end
		m=mm
	end
	
	style=style or {}
	table.clear(self._style)
	table.clone(style,self._style)
	self._style.__index=self._style
	setmetatable(self._style,getmetatable(style))
	
	--Relink styles
	m=self._style
	while true do
		local mm=getmetatable(m)
		if not mm then 
			setmetatable(m,UI.Style)
			break
		end
		m=mm
	end

	if UI.Default.styleCustomSizes then
		UI.Default.styleCustomSizes(self._style)
	end
	stage:setStyle(nil,true)
	if UI.Control then
		UI.Control.DRAG_THRESHOLD=stage:resolveStyle("szDragThreshold",self._style)
		UI.Control.WHEEL_DISTANCE=stage:resolveStyle("szWheelDistance",self._style)
	end
end

local isNumTbl={ 
	["0"]=true,
	["1"]=true,
	["2"]=true,
	["3"]=true,
	["4"]=true,
	["5"]=true,
	["6"]=true,
	["7"]=true,
	["8"]=true,
	["9"]=true,
	["-"]=true,
	["."]=true,
}
local fileSuffix={
	[".png"]=true,
	[".jpg"]=true,
	}

if Sprite.resolveStyle then
	--[[function UI.Style:resolve(n)
		return stage:resolveStyle(n,self)
	end	]]
else
	function Sprite.resolveStyle(sp,n,self,noRefs)
		if not self then self=sp._style end
		local iself=self
		local pself=self.__Parent
		local limit=100
		while limit>0 do
			local nt=type(n)
			if nt~="string" then return n,nt end
			local fc=n:sub(1,1)
			if isNumTbl[fc] then --number
				local num,unit=n:match("([-.0-9]+)(.*)")
				if unit=="s" then
					n=tonumber(num)*UI.Style.fontSize
				elseif unit=="is" then
					n=tonumber(num)*UI.Style.fontSize*UI.Style.iconScale
				elseif unit=="em" then
					n=tonumber(num)*sp:resolveStyle("font",iself):getLineHeight()
				else
					assert(false,"Unit not recognized:"..unit)
				end
				return n,"number"
			elseif fc=="|" or (#n>3 and fileSuffix[n:lower():sub(#n-3)]) then --file
				return n,nt
			end
			if not noRefs then
				local rname=rawget(self,"__Reference")
				if rname then
					local ref=sp:resolveStyle(rname,iself,true)
					assert(ref,"No such reference:"..rname)
					assert(not ref.__Reference,"Referenced style shouldn't contain references")
					self=ref
				end
			end
			local nn=self[n]
			if nn==nil then
				self=pself
				if not self then return end
				pself=self.__Parent
			else 
				n=nn
				self=iself
				pself=self.__Parent
			end
			limit-=1
		end
		assert(limit==0,"Recursion while resolving:"..n)
	end
end

UI.Style.zoomFactor=screenProperties.zoomFactor
UI.Style.fontSize=(UI.Default.fontSize or screenProperties.zoomTarget*screenProperties.zoomFactor)*(UI.Default.fontScale or 1)

local function loadFont(ttf,size,outline)
	local f=TTFont.new(ttf,size,"",true,outline)
	f._size=size
	return f
end
if UI.Default.Fonts then
	for k,fs in pairs(UI.Default.Fonts) do
		UI.Style[k]=loadFont(fs.ttf,UI.Style.fontSize*(fs.size or 1))
	end
elseif UI.Default.TTF then
	UI.Style.font=loadFont(UI.Default.TTF,UI.Style.fontSize)
	UI.Style["font.small"]=loadFont(UI.Default.TTF,UI.Style.fontSize*.7)
	UI.Style["font.bold"]=loadFont(UI.Default.TTF,UI.Style.fontSize,1.1)
else --SystemFont
	UI.Style.font=Font.getDefault()
	UI.Style.font._size=0
	print("UI.Style.font is Font Default ! set UI.Default.TTF=File.ttf or UI.Default.TTF={File1.ttf,File2.ttf} or dependency GiderosLib/GiderosInit/base with _UI = true in init.lua")
end

if debug then 
	print("UI.Style","UI.Style.fontSize",UI.Style.fontSize)
	print("UI.Style","UI.Default.fontSize?",UI.Default.fontSize,"UI.Default.fontScale?",UI.Default.fontScale)
	if _inspect then print("UI.Style","UI.Default.TTF?",_inspect(UI.Default.TTF)) end
end

UI.Style.icon=Texture.new("ui/icons/panel.png",true)
UI.Style.iconScale=1 --Button/icons have the same size as a line of text
if UI.Default.iconScale then UI.Style.iconScale = UI.Default.iconScale end

if UI.Default.styleCustomSizes then
	UI.Default.styleCustomSizes(UI.Style)
end

UI.Style["unit.s"]=UI.Style.fontSize
UI.Style["unit.is"]=UI.Style.fontSize*UI.Style.iconScale

--print("UI Units:","s="..UI.Style["unit.s"],"is="..UI.Style["unit.is"])

-- Statics
UI.Style.colTesting			=UI.Color(1,0.7,0.92,.5) 	--Light pink
UI.Style.colShadow			=UI.Color(0,0,0,.5) 		--black 50%
--Inactive backgrounds
UI.Style.colDeep			=UI.Color(.3,.3,.3,1)		--Very dark grey
UI.Style.colBackground		=UI.Colors.white 			--White
UI.Style.colPane			=UI.Color(.5,.5,.5,1) 		--Dark Grey
-- Active backgrounds
UI.Style.colTile			=UI.Color(.5,.5,.5,1) 		--Dark Grey
UI.Style.colHeader			=UI.Color(.75,.75,.75,1) 	--Grey
UI.Style.colSelect			=UI.Color(0,.75,1,.9) 		--Light blue
-- Composites
--nothing
-- Highlight
UI.Style.colHighlight		=UI.Color(0,.25,1,.9) 		--Dark blue
-- Texts
UI.Style.colUI				=UI.Colors.black			
UI.Style.colText			=UI.Colors.black			--T = T1 ( = T2 ? )
-- Base
UI.Style.colDisabled		=UI.Color(0,0,0,.3) 		--black 30%
-- States
UI.Style.colError			=UI.Color(0xFF0000) 		--red

UI.Style.colWidgetBack		=UI.Colors.transparent
UI.Style.brdWidget			=nil
UI.Style.szDragThreshold	="1s"
UI.Style.szWheelDistance	="1em"

local colNone=UI.Colors.transparent		--TOSEE: maybe create a UI.Color table for standard things ?
local colFull=UI.Colors.white

if UI.Default.styleCustom then
	UI.Default.styleCustom(UI.Style)
end
UI.Style.accordion={
	colSelected="dnd.colSrcHighlight",
	styHeaderSelected={ colWidgetBack="accordion.colSelected" },
}
UI.Style.bar={
	colForeground="colSelect",
	colBackground="colBackground",
}
UI.Style.breadcrumbs={
	styItem={
	},
	styRoot={ 
	},
	styLast={
		colText="colHighlight",
	},
	styElipsis="breadcrumbs.styRoot",
	stySeparator={
		colText="colHeader",
	},
	szSpacing=".1s"
}
UI.Style.button={
	styBack={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={".5s",".5s",".5s",".5s",63,63,63,63,},
			insets=".5s",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="button.colBackground", colLayer2="button.colFocus", colLayer3="button.colSelect", colLayer4=colNone } 
		},
	},
	styInside={
	},
	styError={
		["button.colBackground"]="colError",
	},
	stySelected={
		["button.colSelect"]="colSelect"
	},
	stySelectedFocused={
		["button.colSelect"]="colSelect",
		["button.colFocus"]="colHighlight"
	},
	styFocused={
		["button.colFocus"]="colHighlight"
	},
	styDisabled={
		["button.colFocus"]="colDisabled",
		["button.styInside"]={
			["image.colTint"]="colDisabled",
		}
	},
	colBackground="colHeader",
	colFocus="colUI",
	colSelect=colNone,
}
UI.Style.buttontextfield={
	icButton=Texture.new("ui/icons/panel.png",true,{ mipmap=true }),
	styButton={
		["button.styBack"]={
			colWidgetBack=colNone,
			brdWidget={},
			shader={},
		},
		["button.colBackground"]=colNone,
		["button.colBorder"]=colNone,
		["button.colFocus"]=colNone,
		["button.colSelect"]=colNone,
		["button.styInside"]={
			["image.colTint"]="colText",
		},
		["button.styError"]={
			["button.colBackground"]="colError",
		},
		["button.stySelected"]={
			["button.styInside"]={
				["image.colTint"]="colSelect",
			}
		},
		["button.stySelectedFocused"]={
			["button.styInside"]={
				["image.colTint"]="colSelect",
			}
		},
		["button.styFocused"]={
			["button.styInside"]={
				["image.colTint"]="colText",
			}
		},
	},
}
UI.Style.buttontextfieldcombo={
	icButton="dropdown.icButton",
	styButton="dropdown.styButton",
}
UI.Style.calendar={	
	fontDayNames="font.bold",
	colBackground="colBackground",
	colBorder="colHeader",
	colSpinnerBackground="calendar.colBorder",
	colSpinnerBorder="calendar.colBorder",
	colDays="colText",
	colDaysOther="colPane",
	colDayToday="colPane",
	colDaySelected="colHighlight",
	colDayHeader="colText",
	szDay="1.7em",
	szCellSpacing=".2s",
	szCorner=".5s",
	szInset="0s",
	szMargin=".2s",
	szSpinnerInset=".3s",
	styDayHeader={
		["label.color"]="calendar.colDayHeader",
		["label.font"]="calendar.fontDayNames",
	},
	styDays={
		["label.color"]="calendar.colDays",
	},
	styDaysOther={
		["label.color"]="calendar.colDaysOther",
	},
	styDayToday={
		["label.color"]="calendar.colDays",
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="calendar.colDayToday", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styDaySelected={
		["label.color"]="calendar.colDays",
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="calendar.colDaySelected", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	
	styBase={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"calendar.szCorner","calendar.szCorner","calendar.szCorner","calendar.szCorner",63,63,63,63,},
			insets="calendar.szInset",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="calendar.colBackground", colLayer2="calendar.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	stySpinners={
	},
	stySpinnersLocal={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"calendar.szCorner","calendar.szCorner","calendar.szCorner","calendar.szCorner",63,63,63,63,},
			insets="calendar.szSpinnerInset",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="calendar.colSpinnerBackground", colLayer2="calendar.colSpinnerBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
}
UI.Style.chart={
	colBackground=colNone,
	colItem="colUI",
	colItemSelected="colHighlight",
	icBar=Texture.new("ui/icons/textfield-multi.png",true,{ rawalpha=true, mipmap=true }),
	szBarCorner=".3s",
	szBarCornerTextureRatio=0.5,
	colAxis="colUI",
	szAxisThickness=".1s",
}
UI.Style.checkbox={
	styTickbox={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/checkbox-multi.png",true,{ rawalpha=true, mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="checkbox.colTickboxBack", colLayer2="checkbox.colTickboxFore", colLayer3="checkbox.colTickboxTick", colLayer4="checkbox.colTickboxThird" } 
		}
	},
	colTickboxBack=colNone,
	colTickboxFore="colUI",
	colTickboxTick="colUI",
	colTickboxThird=colNone,
	styUnticked={
		["checkbox.colTickboxTick"]=colNone, 
	},
	styTicked={
	},
	styThird={
		["checkbox.colTickboxTick"]=colNone, 
		["checkbox.colTickboxThird"]="colUI", 
	},
	styDisabled={
		["checkbox.colTickboxTick"]=colNone, 
		["checkbox.colTickboxFore"]="colDisabled", 
	},
	styDisabledTicked={
		["checkbox.colTickboxFore"]="colDisabled", 
		["checkbox.colTickboxTick"]="colDisabled", 
	},
	styDisabledThird={
		["checkbox.colTickboxFore"]="colDisabled", 
		["checkbox.colTickboxTick"]=colNone, 
		["checkbox.colTickboxThird"]="colDisabled", 
	},
	szIcon="1is",
}
UI.Style.colorpicker={
	icPicker=Texture.new("ui/icons/ic_palette.png",true,{ mipmap=true }),
	icValidate=Texture.new("ui/icons/kbd_check.png",true,{ mipmap=true }),
	icCancel=Texture.new("ui/icons/ic_cross.png",true,{ mipmap=true }),
	icCrosshair=Texture.new("ui/icons/ic_reticule.png",true,{ mipmap=true }),
	colBackground="colBackground",
	colBorder="colHeader",
	szCorner=".5s",
	szInset="0.1s",
	szCellSpacing=".2s",
	szHistoWidth="2s",
	szHistoHeight="2s",
	szColorRange="8.6s", --Choosen to fit 4x Histo cells in height (incl. spacing)
	szColorSample="2s",
	szButtonWidth="3.1s", -- Choosen to fit half of 3x Histo cells
	szCrosshair="2s",
	szColorBoxInset=".5s",
	styBase={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"colorpicker.szCorner","colorpicker.szCorner","colorpicker.szCorner","colorpicker.szCorner",63,63,63,63,},
			insets="colorpicker.szInset",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="colorpicker.colBackground", colLayer2="colorpicker.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styColorBox={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/bdr-multi.png",true,{ mipmap=true }),
			corners={"colorpicker.szCorner","colorpicker.szCorner","colorpicker.szCorner","colorpicker.szCorner",63,63,63,63,},
			insets="colorpicker.szInset",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="colorpicker.colBackground", colLayer2="colorpicker.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
}

UI.Style.combobox={
	styBase={
		["button.styBack"]={
			colWidgetBack=colNone,
			brdWidget={},
		},
		["button.styError"]={
			["button.colBackground"]="colError",
		},
		["button.stySelected"]={
			["button.colSelect"]="colSelect"
		},
		["button.stySelectedFocused"]={
			["button.colSelect"]="colSelect",
			["button.colFocus"]="colHighlight"
		},
		["button.styFocused"]={
			["button.colFocus"]="colHighlight"
		},
		["button.colBackground"]="colHeader",
		["button.colFocus"]="colUI",
		["button.colSelect"]=colNone,
		["textfield.styReadonly"]={
		},
	},
	styListContainer={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={".5s",".5s",".5s",".5s",63,63,63,63,},
			insets=".5s",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="combobox.colBackground", colLayer2="combobox.colBorder", colLayer3=colNone, colLayer4=colNone } 
		},		
	},
	colBackground="colBackground",
	colBorder="colUI",
}
UI.Style.comboboxbutton={
	styBase="combobox.styBase",
	styListContainer="combobox.styListContainer",
}
UI.Style.datepicker={
	szWidth="7em",
	styNormal={},
	styError={
		["label.color"]="colError",
		["textfield.colForeground"]="colError",
	},
	icPicker=Texture.new("ui/icons/ic_cal.png",true,{ mipmap=true }),
}
UI.Style.dialog={
	colBackground="colBackground",
	szInset=".5s",
	szSpacing=".5s",
	szButtonMargin=".5s"
}

UI.Style.dnd={
	colSrcHighlight=UI.Color(0,.25,1,.9,.15),
	colDstHighlight="colHighlight",
	colDstHighlightOver=UI.Color(0,.25,1,.9,.5),
	szInsertPoint=".3s",
	szMarkerMargin=".4s",
	szMarkerInset=".3s",
	styMarker={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/textfield-multi.png",true,{ mipmap=true }),
			corners={"dnd.szMarkerMargin","dnd.szMarkerMargin","dnd.szMarkerMargin","dnd.szMarkerMargin",39,39,39,39},
			insets="dnd.szMarkerInset",
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="dnd.colMarkerBack", colLayer2=colNone, colLayer3="dnd.colMarkerBorder", colLayer4=colNone }
		},
	},
	styMarkerDenied={
		["dnd.colMarkerBorder"]="colDisabled",
	},
	colMarkerBack=colNone,
	colMarkerBorder="colHighlight",
	colMarkerTint=UI.Color(1,1,1,.85),
}
UI.Style.dropdown={
	icButton=Texture.new("ui/icons/rdown.png",true,{ mipmap=true }),
	styButton="buttontextfield.styButton",
}
UI.Style.editableclock={	
	colBackground="calendar.colBackground",
	colBorder="calendar.colBorder",
	
	colRing="colPane",
	colCenter="editableclock.colBackground",
	colDot="editableclock.colBorder",
		
	fontLabels="font",	
	colLabels="colText",
	
	szClock="10is",
	szRing="9.5is",
	szText="9is",
	szCenter="7is",
	szDot=".6s",
	
	szCorner=".5s",
	szInset=".2s",
	szMargin=".2s",

	styLabel={
		["label.color"]="editableclock.colLabels",
	},
	
	styBase={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"editableclock.szCorner","editableclock.szCorner","editableclock.szCorner","editableclock.szCorner",63,63,63,63,},
			insets="editableclock.szInset",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="editableclock.colBackground", colLayer2="editableclock.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styHandBase={		
		["handle.styBase"]={
			colWidgetBack=colFull,
			brdWidget="handle.marker",
			shader={ 
				class="UI.Shader.MultiLayer", 
				params={ colLayer1="hand.colHand", colLayer2="handle.colText", colLayer3=colNone, colLayer4=colNone } 
			}
		},
		["handle.styArrows"]={
			colWidgetBack="hand.colArrows",
			brdWidget=UI.Border.NinePatch.new({
				texture=Texture.new("ui/icons/clock_arrows.png",true,{ mipmap=true }),
				corners={0,0,0,0,0,0,0,0},
			}),
		},
		["handle.colText"]="colText",
		["handle.szHandle"]="2is",
		["hand.colHand"]="colHighlight",
		["hand.colArrows"]="colUI",
		["hand.szWidth"]=".1s",
		
		colWidgetBack="hand.colHand",
	},
	styHandH={		
		["handle.marker"]=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/clock_hand_h.png",true,{ mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
		}),
		["hand.szHand"]="3is",
		["hand.szOffset"]=".5s",
		["handle.szOffset"]="2is",
	},
	styHandM={		
		["handle.marker"]=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/clock_hand_m.png",true,{ mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
		}),
		["hand.szHand"]="4.5is",
		["hand.szOffset"]=".5s",
		["handle.szOffset"]="3.5is",
	},
	styHandSel={
		["hand.colHand"]="colSelect",
	}
}

UI.Style.hilitpanel={
	colHighlight="colHighlight",
	styNormal={
	},
	styHighlight={
		colWidgetBack="hilitpanel.colHighlight"
	}
}

UI.Style.image={
	colTint=colFull,
	szIcon="1is",
}
UI.Style.keyboard={
	colSpecKeys="colHighlight",
	szSpacing=".5s",
	szKey="2s",
	szLineHeight="2is",
	szMargin=".5s",
	szSpecKey="6s",
	szEnterKey="13s",
	icBS=Texture.new("ui/icons/kbd_bs.png",true,{ mipmap=true }),
	icOK=Texture.new("ui/icons/kbd_ocheck.png",true,{ mipmap=true }),
	icShift=Texture.new("ui/icons/kbd_shift.png",true,{ mipmap=true }),
	icUnShift=Texture.new("ui/icons/kbd_noshift.png",true,{ mipmap=true }),
	icHide=Texture.new("ui/icons/kbd_hide.png",true,{ mipmap=true }),
	icCaps=Texture.new("ui/icons/kbd_capslock.png",true,{ mipmap=true }),
	styKeys={
	},
	stySpecKeys={
		["button.colBackground"]="keyboard.colSpecKeys",
	},
	styBase={
		colWidgetBack="colBackground",
	}
}
UI.Style.label={
	color="colText",
	szInset=".2s",
	font="font",
}
UI.Style.passwordfield={
	icButton=Texture.new("ui/icons/eye.png",true,{ mipmap=true }),
	styButton="buttontextfield.styButton",
}
UI.Style.progress={
	szCircular="2is",
	brdCircular=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/spot.png",true,{ mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
			insets="progress.szCircular",
	}),
	styBeads={
		["progress.szBead"]=".5s",
		["progress.szBeadMargin"]=".15s",
		["progress.colBead"]="colText",
		--["progress.icBead"]=Texture.new("ui/icons/eye.png",true,{ mipmap=true }),
	},
	styCircular={
		colWidgetBack="colHighlight",
		
		brdWidget="progress.brdCircular",
		shader={ 
			class="UI.Shader.SectorPainter", 
			params={
				numAngleStart=0,
				--numAngleEnd=0,
				numRadiusStart=0,
				numRadiusEnd=1,
				colIn=0xFFFFFF, --White
				colOut=#0
			}
		},
	},
	colBorder="colUI",
	colBackground="colBackground",
	colDone="colHighlight",
	colRemain=colNone,
	styBar={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={".5s",".5s",".5s",".5s",63,63,63,63,},
		}),
		shader={ 
			class="UI.Shader.ProgressMultiLayer", 
			params={ colLayer1="progress.colBackground", colLayer2="progress.colBorder", colLayer3="progress.colDone", colLayer3a="progress.colRemain", colLayer4=colNone } 
		},		
	},
	styBarText={
		colWidgetBack=colNone,
	},
	styNormal={
	},
	styDisabled={
		["progress.colBackground"]="colDisabled",
		["progress.colDone"]="colDisabled",
	},
}
UI.Style.radio={
	styTickbox={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="radio.colTickboxBack", colLayer2="radio.colTickboxFore", colLayer3="radio.colTickboxTick", colLayer4=colNone } 
		}
	},
	colTickboxBack=colNone,
	colTickboxFore="colUI",
	colTickboxTick="colUI",
	styUnticked={
		["radio.colTickboxTick"]=colNone, 
	},
	styTicked={
	},
	styDisabled={
		["radio.colTickboxTick"]=colNone, 
		["radio.colTickboxFore"]="colDisabled", 
	},
	styDisabledTicked={
		["radio.colTickboxFore"]="colDisabled", 
		["radio.colTickboxTick"]="colDisabled", 
	},
	szIcon="1is",
}
UI.Style.scrollbar={
	styBar={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob",63,63,63,63,},
			insets=0,
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="scrollbar.colBar", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	szKnob=".3s",
	styKnob={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true),
			corners={"scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob",63,63,63,63,},
			insets="scrollbar.szKnob",
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="scrollbar.colKnob", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	colBar="colHeader",
	colKnob="colHighlight",
}
UI.Style.slider={
	colKnob="colHighlight",
	colKnobCenter="colText",
	styHand={
		colWidgetBack="colText",
		["slider.szHand"]=".1s",
	},
	colRailBorder="colHeader",
	colRailBackground="colBackground",
	colRailActive="colSelect",
	colRailInactive=colNone,
	szRail=".5s",
	szKnob="2s",
	styKnob={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={0,0,0,0,0,0,0,0},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="slider.colKnobCenter", colLayer2="slider.colKnob", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styRail={
		["slider.railTexture"]=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
		["slider.railCorners"]={".5s",".5s",".5s",".5s",63,63,63,63,},
		shader={ 
			class="UI.Shader.MeshLineMultiLayer", 
			params={ colLayer1="slider.colRailBackground", colLayer2="slider.colRailBorder", colLayer3="slider.colRailActive", colLayer3a="slider.colRailInactive", colLayer4=colNone } 
		},		
	},
	styNormal={
	},
	styDisabled={
		["slider.colRailActive"]=colNone,
	},
	styKnobNormal={
	},
	styKnobDisabled={
		["slider.colKnob"]="colDisabled",
		["slider.colKnobCenter"]="colDisabled",
	},
}
UI.Style.spinner={
	icNumPrev=Texture.new("ui/icons/minus.png",true,{ mipmap=true }),
	icNumNext=Texture.new("ui/icons/plus.png",true,{ mipmap=true }),
	icLstPrev=Texture.new("ui/icons/rprev.png",true,{ mipmap=true }),
	icLstNext=Texture.new("ui/icons/rnext.png",true,{ mipmap=true }),
	szIcon="1is",
	colIcon="colHighlight",
	colIconDisabled="colDisabled",
	styNormal={
	},
	styDisabled={
	},
	styButtonNormal={
		["image.colTint"]="spinner.colIcon",
	},
	styButtonDisabled={
		["image.colTint"]="spinner.colIconDisabled",
	},
	styTextNormal={
	},
	styTextDisabled={
	},
}
UI.Style.splitpane={
	szKnob="1is",
	brdKnobH=UI.Border.NinePatch.new({
		texture=Texture.new("ui/icons/splitpane-bar-h.png",true,{ mipmap=true }),
		corners={0,0,"1s","1s",0,0,16,16},
	}),
	brdKnobV=UI.Border.NinePatch.new({
		texture=Texture.new("ui/icons/splitpane-bar-v.png",true,{ mipmap=true }),
		corners={"1s","1s",0,0,16,16,0,0},
	}),
	tblKnobSizes={".9is",".2is",".9is"},
	colKnobBackground=colNone,
	colKnob="colHeader",
	colKnobHandle="colHighlight",
	colKnobSymbol="colUI",
	colKnobShadow="colShadow",
	styKnobBackgroundH={
		colWidgetBack="splitpane.colKnobBackground",
	},
	styKnobBackgroundV={
		colWidgetBack="splitpane.colKnobBackground",
	},
	styKnobH={
		brdWidget="splitpane.brdKnobH",
		colWidgetBack="splitpane.colKnob" 
	},
	styKnobV={
		brdWidget="splitpane.brdKnobV",
		colWidgetBack="splitpane.colKnob" 
	},
	styKnobHandleH={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/splitpane-h.png",true,{ mipmap=true, rawalpha=true }),
			corners={0,0,0,0,0,0,0,0},
			insets="splitpane.szKnob",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="splitpane.colKnobHandle", colLayer2="splitpane.colKnobSymbol", colLayer3="splitpane.colKnobShadow" }},
	},
	styKnobHandleV={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/splitpane-v.png",true,{ mipmap=true, rawalpha=true }),
			corners={0,0,0,0,0,0,0,0},
			insets="splitpane.szKnob",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="splitpane.colKnobHandle", colLayer2="splitpane.colKnobSymbol", colLayer3="splitpane.colKnobShadow" }},
	},
}
UI.Style.tabbedpane={
	szInset=0,
	szCorner=".5s",
	szPaneCorner=".3s",
	szPaneCornerTop=0,
	colBackground="colBackground",
	colOutline="colHighlight",
	colTabBackground="colTile",
	colTabBorder="colUI",
	styPane={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-b.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets={ left="tabbedpane.szPaneCorner", right="tabbedpane.szPaneCorner", top="tabbedpane.szPaneCornerTop", bottom="tabbedpane.szPaneCorner" },
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colBackground", colLayer2="tabbedpane.colOutline", colLayer3=colNone, }},
	},
	styHLine={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-l.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets="tabbedpane.szCorner",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colBackground", colLayer2="tabbedpane.colOutline", colLayer3=colNone, }},
	},
	styTabCurrent={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-t.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets="tabbedpane.szCorner",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colBackground", colLayer2="tabbedpane.colOutline", colLayer3=colNone, }},
	},
	styTabCurrentFirst={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-tf.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets="tabbedpane.szCorner",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colBackground", colLayer2="tabbedpane.colOutline", colLayer3=colNone, }},
	},
	styTabCurrentLast={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-tl.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets="tabbedpane.szCorner",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colBackground", colLayer2="tabbedpane.colOutline", colLayer3=colNone, }},
	},
	styTabOther={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/tab-t.png",true,{ mipmap=true }),
			corners={"tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner","tabbedpane.szCorner",40,40,40,40},
			insets="tabbedpane.szCorner",
		}),
		colWidgetBack=0xFFFFFF,
		shader={ class="UI.Shader.MultiLayer", params={ colLayer1="tabbedpane.colTabBackground", colLayer2="tabbedpane.colTabBorder", colLayer3=colNone, }},
	},
}
UI.Style.table={
	colHeader="colHeader",
	colTextHeader="colHighlight",
	styDndMarker={
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={".5s",".5s",".5s",".5s",63,63,63,63,},
			insets=".5s",
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="table.colHeader", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone }
		},
		colWidgetBack="table.colHeader",
		colText="table.colTextHeader",
	},
	szResizeHandle="1s", --Size of column resize handle
	styRowHeader={ colText="table.colTextHeader" },
	styRowHeaderLocal={ colWidgetBack="table.colHeader" },
	styRowSelected={ colWidgetBack="colSelect" },
	styRowOdd={ },
	styRowEven={ },
	styCell={ },
	styCellSelected={ },
}
UI.Style.textfield={
	styBase={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/textfield-multi.png",true,{ mipmap=true }),
			corners={"textfield.szMargin","textfield.szMargin","textfield.szMargin","textfield.szMargin",63,63,63,63,},
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="textfield.colBackground", colLayer2="textfield.colBorder", colLayer3="textfield.colBorderWide", colLayer4=colNone } 
		}
	},
	colForeground="colText",
	colTipText="colPane",
	colBackground="colBackground",
	colBorder="colUI",
	colBorderWide=colNone,
	colSelection="colSelect",
	styNormal={
	},
	styDisabled={
		["textfield.colBackground"]=colNone, 
		["textfield.colBorder"]="colDisabled", 
		["textfield.colBorderWide"]=colNone, 
		["textfield.colForeground"]="colDisabled", 
	},
	styFocused={
		["textfield.colBorder"]="colHighlight", 
	},
	styFocusedReadonly={
		["textfield.colBorder"]="colHighlight", 
	},
	styReadonly={
	},
	szMargin=".3s",
	szCutPasteButton="1is",
	colCutPasteButton="colText",
	styCutPaste={
		["image.colTint"]="textfield.colCutPasteButton",
		["textfield.szSpacing"]=".2s",		
		["textfield.colBoxBack"]="colBackground",
		["textfield.colBoxBorder"]="colHighlight",
		["textfield.szBoxBorder"]=".3s",
		["textfield.icCut"]=Texture.new("ui/icons/cp_cut.png",true,{ mipmap=true }),
		["textfield.icCopy"]=Texture.new("ui/icons/cp_copy.png",true,{ mipmap=true }),
		["textfield.icPaste"]=Texture.new("ui/icons/cp_paste.png",true,{ mipmap=true }),
	},
	styCutPasteBox={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"textfield.szBoxBorder","textfield.szBoxBorder","textfield.szBoxBorder","textfield.szBoxBorder",63,63,63,63,},
			insets="textfield.szBoxBorder",
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="textfield.colBoxBack", colLayer2="textfield.colBoxBorder", colLayer3=colNone, colLayer4=colNone }
		},
	},
	styCutPasteSingle={
		["textfield.szSpacing"]=0,
	},
}
UI.Style.timepicker={
	szWidth="5em",
	styNormal={},
	styError={
		["label.color"]="colError",
		["textfield.colForeground"]="colError",
	},
	icPicker=Texture.new("ui/icons/ic_clock.png",true,{ mipmap=true }),
}
UI.Style.toolbox={
	styToolbox={
		["table.styRowHeader"]="toolbox.styHeader",
		["table.styRowHeaderLocal"]="toolbox.styHeaderLocal",
		["table.styCell"]="toolbox.styItem",
	},
	styContainer={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/cirbdr-multi.png",true,{ mipmap=true }),
			corners={"toolbox.szBorder","toolbox.szBorder","toolbox.szBorder","toolbox.szBorder",63,63,63,63},
			insets=0,
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="toolbox.colBack", colLayer2="toolbox.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styItem={},
	styHeader={
		["image.colTint"]="toolbox.colHeaderIcon",
	},
	styHeaderLocal={
		colWidgetBack=colFull,
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="toolbox.colHeader", colLayer2="toolbox.colBorder", colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styHeaderHorizontal={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/cirbdr-multi.png",true,{ mipmap=true }),
			corners={"toolbox.szBorder",0,"toolbox.szBorder","toolbox.szBorder",63,63,63,63},
			insets=0,
		}),
	},
	styHeaderVertical={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/cirbdr-multi.png",true,{ mipmap=true }),
			corners={"toolbox.szBorder","toolbox.szBorder","toolbox.szBorder",0,63,63,63,63},
			insets=0,
		}),
	},
	colBack="colBackground",
	colHeader="colHeader",
	colHeaderIcon="colBackground",
	colBorder="colHighlight",
	szBorder=".3s",
}
UI.Style.toolpie={
	colRing1="textfield.colBackground", 
	colRing2="textfield.colBorder", 
	colRing3="textfield.colBorderWide",
	colRing4=colNone,
	txRing=Texture.new("ui/icons/textfield-multi.png",true,{ rawalpha=true, mipmap=true }),

	colBackground=colNone,
	colItem="colUI",
	colItemSelected="colHighlight",
	szBarCorner=".3s",
	szBarCornerTextureRatio=0.5,
	colAxis="colUI",
	szAxisThickness=".1s",
}
UI.Style.tooltip={
	szOffsetMax="6s",
	styMarker={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={".5s",".5s",".5s",".5s",63,63,63,63,},
			insets=".1s",
		}),
		shader={
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="tooltip.colMarkerBack", colLayer2="tooltip.colMarkerBorder", colLayer3=colNone, colLayer4=colNone }
		},
	},
	colMarkerBack="colBackground",
	colMarkerBorder="colHighlight",
}
UI.Style.tree={
	icExpand=Texture.new("ui/icons/tree-expand.png",true,{ mipmap=true }),
	icCollapse=Texture.new("ui/icons/tree-collapse.png",true,{ mipmap=true }),
	icVert=Texture.new("ui/icons/tree-vert.png",true,{ mipmap=false }),
	icSub=Texture.new("ui/icons/tree-sub.png",true,{ mipmap=true }),
	icEnd=Texture.new("ui/icons/tree-end.png",true,{ mipmap=true }),
	colLine="colUI",
	szBox="1s",
	stySubCell={
	},
	stySub={
		colWidgetBack="tree.colLine",
		brdWidget=UI.Border.NinePatch.new({
			texture="tree.icSub",
			corners={0,0,0,0,0,0,0,0},
		}),
	},
	styEnd={
		colWidgetBack="tree.colLine",
		brdWidget=UI.Border.NinePatch.new({
			texture="tree.icEnd",
			corners={0,0,0,0,0,0,0,0},
		}),
	},
	styCollapse={
		colWidgetBack="tree.colLine",
		brdWidget=UI.Border.NinePatch.new({
			texture="tree.icCollapse",
			corners={0,0,0,0,0,0,0,0},
		}),
	},
	styExpand={
		colWidgetBack="tree.colLine",
		brdWidget=UI.Border.NinePatch.new({
			texture="tree.icExpand",
			corners={0},
		}),
	},
	styVert={
		colWidgetBack="tree.colLine",
		brdWidget=UI.Border.NinePatch.new({
			texture="tree.icVert",
			corners={0,0,1,0,0,0,1,0},
		}),
	},
}
UI.Style.weekschedule={	
	colGrid="colHeader",
	colGridFirst="colText",
	fontLabel="font.small",
	colLabelBack="colHeader",
	colBars="colHighlight",
	colCell="colBackground",
	colHeader="colHeader",
	szCell="1.5em",
	szCellSpacing=".2s",
	styHeader={
		brdWidget=nil,
		colWidgetBack="weekschedule.colHeader",
	},
	styBars={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob",63,63,63,63,},
			insets=0,
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="weekschedule.colBars", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styLabel={
		colWidgetBack=colFull,
		brdWidget=UI.Border.NinePatch.new({
			texture=Texture.new("ui/icons/radio-multi.png",true,{ mipmap=true }),
			corners={"scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob","scrollbar.szKnob",63,63,63,63,},
			insets=0,
		}),
		shader={ 
			class="UI.Shader.MultiLayer", 
			params={ colLayer1="weekschedule.colLabelBack", colLayer2=colNone, colLayer3=colNone, colLayer4=colNone } 
		}
	},
	styNormal={
	},
	styDisabled={
		["weekschedule.colBars"]="colDisabled",
		["weekschedule.colLabelBack"]="colDisabled",
		["weekschedule.colHeader"]="colDisabled",
	},
}

if UI.Default.styleCustomWidgets then
	UI.Default.styleCustomWidgets(UI.Style)
end
