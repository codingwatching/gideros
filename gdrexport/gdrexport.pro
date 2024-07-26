#-------------------------------------------------
#
# Project created by QtCreator 2013-05-16T12:38:10
#
#-------------------------------------------------

QT       += core network xml

#QT       -= gui

TARGET = gdrexport
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += silent

TEMPLATE = app

LUA_ENGINE=$$(LUA_ENGINE)
isEmpty(LUA_ENGINE): LUA_ENGINE=luau
equals(LUA_ENGINE,luau): DEFINES+=USE_LUAU_ENGINE
equals(LUA_ENGINE,lua): 


SOURCES += main.cpp GAppFormat.cpp Utilities.cpp WinRTExport.cpp MacOSXExport.cpp \
	ExportCommon.cpp ExportBuiltin.cpp ExportXml.cpp filedownloader.cpp ExportLua.cpp \
	ExportScript.cpp
HEADERS += GAppFormat.h Utilities.h WinRTExport.h MacOSXExport.h ExportCommon.h \
	ExportBuiltin.h ExportXml.h filedownloader.h ExportLua.h \
	ExportScript.h

INCLUDEPATH += ../ui ../libnetwork ../2dsg 

HEADERS += \
    ../ui/projectproperties.h \
    ../2dsg/orientation.h \
    ../ui/dependencygraph.h

SOURCES += \
    ../ui/projectproperties.cpp \
    ../ui/dependencygraph.cpp \
    ../libnetwork/bytebuffer.cpp

DEFINES += DESKTOP_TOOLS 
INCLUDEPATH += ../lua/src
SOURCES += \
	../lua/src/lapi.c \
	../lua/src/lauxlib.c \
	../lua/src/lcode.c \
	../lua/src/ldebug.c \
	../lua/src/ldo.c \
	../lua/src/ldump.c \
	../lua/src/lfunc.c \
	../lua/src/llex.c \
	../lua/src/lmem.c \
	../lua/src/lobject.c \
	../lua/src/lopcodes.c \
	../lua/src/lparser.c \
	../lua/src/lstate.c \
	../lua/src/lstring.c \
	../lua/src/ltable.c \
	../lua/src/ltm.c \
	../lua/src/lundump.c \
	../lua/src/lvm.c \
	../lua/src/lzio.c \
	../lua/src/lgc.c \
	../lua/src/linit.c \
	../lua/src/lbaselib.c \
	../lua/src/ldblib.c \
	../lua/src/liolib.c \
	../lua/src/lmathlib.c \
	../lua/src/loslib.c \
	../lua/src/ltablib.c \
	../lua/src/lstrlib.c \
	../lua/src/lutf8lib.c \
	../lua/src/lint64.c \
	../lua/src/loadlib.c

INCLUDEPATH += ./luaext
SOURCES += \
	./luaext/lfs.cpp

win32 {
SOURCES += \
	./luaext/hkey.cpp
}

LIBS += -lz

macx {
	LIBS += -framework CoreFoundation
	LIBS += -framework IOKit
}

unix:!macx {
	QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN\''
}
