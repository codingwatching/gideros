TARGET=imgui

INCS = ../../../Sdk/include
INCS += ../../../Sdk/include/gideros
INCS += ../../../2dsg
INCS += ../../../2dsg/gfxbackends
INCS += ../../../libgideros
INCS += ../../../libgid/include
INCS += ../../../luabinding
INCS += ../../../$(LUA_INCLUDE)
INCS += Common/imgui_src
INCS += Common

SRCS= \
        Common/imgui_src/imgui \
        Common/imgui_src/imgui_demo \
        Common/imgui_src/imgui_draw \
        Common/imgui_src/imgui_widgets \
        Common/imgui_src/imgui_tables \
	   	Common/imgui_user \
	   	Common/TextEditor \
	   	../../../2dsg/Matrices \
	   	../../../2dsg/mouseevent \
	   	../../../2dsg/touchevent \
	   	../../../2dsg/keyboardevent \
        Common/imgui_bindings

#LIBS += -L"../../../Sdk/lib/desktop" -llua -lgid -lgideros -lgvfs

include $(ROOT)/scripts/GidWin32Plugin.mk
