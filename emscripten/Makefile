CORE_SRCS=$(addprefix 2dsg/,$(notdir $(basename $(wildcard ../2dsg/*.cpp))))
CORE_SRCS+=$(addprefix 2dsg/gfxbackends/,$(notdir $(basename $(wildcard ../2dsg/gfxbackends/*.cpp))))
CORE_SRCS+=$(addprefix 2dsg/gfxbackends/gl2/,$(notdir $(basename $(wildcard ../2dsg/gfxbackends/gl2/*.cpp))))
CORE_SRCS+=$(addprefix 2dsg/paths/,$(notdir $(basename $(wildcard ../2dsg/paths/*.cpp)) svg-path ft-path))
CORE_SRCS+=$(addprefix 2dsg/paths/,$(notdir $(basename $(wildcard ../2dsg/paths/*.cpp))))

#LIQUIDFUN=external/liquidfun-1.0.0/liquidfun/Box2D/Box2D
#B2D_SRCS=$(addprefix $(LIQUIDFUN)/Common/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Common/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Collision/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Collision/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Collision/Shapes/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Collision/Shapes/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Dynamics/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Dynamics/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Dynamics/Contacts/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Dynamics/Contacts/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Dynamics/Joints/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Dynamics/Joints/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Rope/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Rope/*.cpp))))
#B2D_SRCS+=$(addprefix $(LIQUIDFUN)/Particle/,$(notdir $(basename $(wildcard ../$(LIQUIDFUN)/Particle/*.cpp))))

LBND_SRCS=$(addprefix luabinding/,$(notdir $(basename $(wildcard ../luabinding/*.cpp)))) luabinding/tlsf
LSUP_SRCS=$(addprefix libnetwork/,$(notdir $(basename $(wildcard ../libnetwork/*.cpp))))
LSUP_SRCS+=$(addprefix libpvrt/,$(notdir $(basename $(wildcard ../libpvrt/*.cpp))))
LSUP_SRCS+=$(addprefix external/glu/libtess/,$(notdir $(basename $(wildcard ../external/glu/libtess/*.c))))
LSUP_SRCS+=$(addprefix external/minizip-1.1/source/,ioapi unzip)
LGID_SRCS=$(addprefix libgid/src/,aes md5 platformutil utf8 drawinfo gtimer \
	gimage gimage-png gimage-jpg gtexture gevent glog gglobal gaudio \
	gaudio-loader-wav \
	gvfs-native)
LGID_SRCS+=$(addprefix libgideros/,event eventdispatcher eventvisitor refptr stringid luautil pluginmanager binderutil)


#    ../libgid/src/qt/platform-qt.cpp \

LGVFS_SRCS=$(addprefix libgvfs/,$(notdir $(basename $(wildcard ../libgvfs/*.c))))
LGVFS_SRCS+=$(addprefix libgvfs/,$(notdir $(basename $(wildcard ../libgvfs/*.cpp))))
ifeq ($(LUA_ENGINE),luau)
LUA_INCLUDE_ALL= $(addprefix luau/,VM/src VM/include Compiler/include Ast/include Common/include)
LSUP_SRCS+= \
        $(addprefix luau/VM/src/,lapi laux lbaselib lbitlib lbuffer lbuflib lbuiltins lcorolib ldblib ldebug ldo lfunc lgc lgcdebug linit lint64lib liolib lmathlib lmem lnumprint lobject loslib lperf lstate lstring lstrlib \
         ltable ltablib ltm ludata lutf8lib lvmexecute lvmload lvmutils) \
        $(addprefix luau/Compiler/src/,Builtins BuiltinFolding BytecodeBuilder ConstantFolding Compiler CostModel lcode PseudoCode TableShape Types ValueTracking) \
        $(addprefix luau/Ast/src/,Ast Confusables Lexer Location Parser StringUtils TimeTrace)
else
LSUP_SRCS+=lua/etc/all_lua
endif
LSUP_SRCS+=libpystring/pystring

FREETYPE_VER=2.7.1
LFRT_SRCS= $(addprefix libgid/external/freetype-$(FREETYPE_VER)/src/, \
	$(addprefix base/,ftbbox ftbitmap ftglyph ftlcdfil ftstroke ftbase ftsystem ftinit ftgasp) \
	raster/raster sfnt/sfnt smooth/smooth autofit/autofit truetype/truetype cff/cff gzip/ftgzip  \
	psnames/psnames pshinter/pshinter)

LZIP_SRCS=$(addprefix libgid/external/zlib-1.2.8/,$(notdir $(basename $(wildcard ../libgid/external/zlib-1.2.8/*.c))))
PNGFILES=png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans \
	pngwio pngwrite pngwtran pngwutil 
LPNG_SRCS=$(addprefix libgid/external/libpng-1.6.2/,$(PNGFILES))
JPEGFILES=jaricom jdapimin jdapistd jdarith jdatadst jdatasrc jdcoefct jdcolor jddctmgr jdhuff jdinput \
	jdmainct jdmarker jdmaster jdmerge jdpostct jdsample jdtrans jerror jfdctflt jfdctfst jfdctint \
	jidctflt jidctfst jidctint jquant1 jquant2 jutils jmemmgr jmemnobs jcomapi \
	jcapimin jcapistd jcarith jccoefct jccolor jcdctmgr jchuff jcinit jcmainct \
	jcmarker jcmaster jcparam jcprepct jcsample jctrans
LJPG_SRCS=$(addprefix libgid/external/jpeg-9/,$(JPEGFILES))
LSUP_SRCS+=$(addprefix libgid/external/snappy-1.1.0/,snappy snappy-c snappy-sinksource snappy-stubs-internal)
MP3FILES=compat dct64 dither equalizer feature format frame icy icy2utf8 id3 index layer1 layer2 layer3 \
	libmpg123 ntom optimize parse readers stringbuf synth synth_8bit synth_real synth_s32 tabinit
LMP3_SRCS=$(addprefix libgid/external/mpg123-1.15.3/src/libmpg123/,$(MP3FILES)) libgid/src/gaudio-loader-mp3
CORE_SRCS+=$(addprefix emscripten/,applicationmanager gapplication-js \
	ginput-js gaudio-js gaudio-stream-openal-js gaudio-sample-openal-js platform-js ghttp-js \
	gui-js ggeolocation-js main jsnative gwebxr cJSON netplayer plugin_support)   

LOCAL_CFLAGS+=-D_REENTRANT -DLIBXMP_CORE_PLAYER
XMP_SRC=virtual period player read_event dataio lfo envelope \
		scan control filter effects mixer mix_all load_helpers load \
		hio smix memio
XMP_HDR=common effects envelope format lfo list mixer period player \
		virtual precomp_lut hio memio mdataio tempfile 
XMP_LOADERS=xm_load s3m_load it_load \
			common itsex sample
XMP_LOADERS_HDR=it loader mod s3m xm
LXMP_SRCS+=$(addprefix libgid/external/libxmp-4.3/src/,$(XMP_SRC))
LXMP_SRCS+=$(addprefix libgid/external/libxmp-4.3/src/loaders/,$(XMP_LOADERS))
LXMP_SRCS+=$(addprefix libgid/external/libxmp-4.3/lite/src/,format loaders/mod_load)
LXMP_SRCS+=libgid/src/gaudio-loader-xmp 

LOAL_SRCS+=$(addprefix libgid/external/openal-soft/, \
			$(addprefix al/,auxeffectslot buffer effect error event extension filter listener source state) \
			$(addprefix al/effects/,autowah chorus compressor convolution dedicated distortion echo effects equalizer fshifter modulator null pshifter reverb vmorpher) \
			$(addprefix alc/,alc alconfig alu context device panning) \
			$(addprefix alc/effects/,autowah chorus compressor convolution dedicated distortion echo equalizer fshifter modulator null pshifter reverb vmorpher) \
			$(addprefix alc/backends/,base null loopback webaudio) \
			$(addprefix common/,alcomplex alfstream almalloc alstring polyphase_resampler ringbuffer strutils threads) \
			$(addprefix core/,ambdec ambidefs bformatdec bs2b bsinc_tables buffer_storage context converter cpu_caps devformat device effectslot except fmt_traits fpu_ctrl helpers hrtf logging mastering mixer uhjfilter uiddefs voice) \
			$(addprefix core/filters/,biquad nfc splitter) \
			$(addprefix core/mixer/,mixer_c) \
	)					


INCS=2dsg libgideros libgid/include libgvfs libpystring
INCS+=2dsg/gfxbackends 2dsg/gfxbackends/gl2 2dsg/paths
INCS+=luabinding libnetwork
INCS+=$(addprefix libgid/external/freetype-$(FREETYPE_VER)/,include src)
INCS+=$(LIQUIDFUN)/.. external/glu
INCS+=libgid/external/zlib-1.2.8 libgid/external/libpng-1.6.2 \
	libgid/external/openal-soft/include \
	libgid/external/snappy-1.1.0 libgid/external/jpeg-9 \
	libgid/external/mpg123-1.15.3/src \
	libgid/external/mpg123-1.15.3/src/libmpg123 \
	libgid/external/libxmp-4.3/include \
	libgid/external/libxmp-4.3/src \
	libgid/external/libxmp-4.3/src/loaders 
INCS+=$(LUA_INCLUDE) $(LUA_INCLUDE_CORE) $(LUA_INCLUDE_ALL)
INCS+=libgvfs/private
INCS+=emscripten

#LIBS+=prebuilt/libfreetype.so

EMCC=$(EMSDK_PREFIX) emcc
EMAR=$(EMSDK_PREFIX) emar
CRUNCHME?=crunchme-linux-amd64
CRUNCHME_TOOL=crunchme-0.4/bin/$(CRUNCHME)
BUILD=Build

#DEBUG=1
POPTS=#-pthread 
#POPTS+= -s PRECISE_F32=1
ifneq ($(PROFILING),)
POPTS+=--profiling
endif

OPTS+=-O2 $(POPTS)
LOPTS=-disable-inlining
LOPTS_WASM=$(LOPTS) -fwasm-exceptions
LOPTS_JS=$(LOPTS) #--separate-asm

export POPTS

$(BUILD)/emscripten/plugin_support.em.o: CFLGS+=-disable-inlining -fno-rtti -fno-exceptions 

NO_EXCEPTIONS?=0
$(BUILD)/2dsg/%.emw.o: NO_EXCEPTIONS=1
$(BUILD)/libgvfs/%.emw.o: NO_EXCEPTIONS=1
$(BUILD)/libpystring/%.emw.o: NO_EXCEPTIONS=1
$(BUILD)/libgid/external/%.emw.o: NO_EXCEPTIONS=1

#LINE BELOW FOR WASM
EOPTS_WASM+= INITIAL_MEMORY=256MB STACK_SIZE=8MB ALLOW_MEMORY_GROWTH=1 #BINARYEN_TRAP_MODE="clamp" #BINARYEN_METHOD="native-wasm" 
EOPTS+=EXPORTED_RUNTIME_METHODS="['addRunDependency','removeRunDependency','ccall','cwrap','getValue','UTF8ToString','loadDynamicLibrary']"
#EOPTS+=LEGACY_RUNTIME
#EOPTS+=EMCC_FORCE_STDLIBS
#EOPTS+=EXPORT_ALL=1
EOPTS+=MAIN_MODULE=2  
EOPTS+=ASSERTIONS=0
#EOPTS+=GL_STATE_CACHE=1
#EOPTS+=DISABLE_EXCEPTION_CATCHING=0
EOPTS+=PROXY_POSIX_SOCKETS=0
#EOPTS+=ALLOW_MEMORY_GROWTH=1
#EOPTS+=USE_WEBGL2=1 WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1
EOPTS+=MAX_WEBGL_VERSION=2
#EOPTS+=USE_GLFW=3
#EOPTS+=ERROR_ON_UNDEFINED_SYMBOLS=0
#EOPTS+=LINKABLE=1
EOPTS+=FORCE_FILESYSTEM=1
EOPTS+=DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$$ALLOC_STACK,$$allocate'
ifneq ($(DEBUG),)
OPTS+=-gsource-map
EOPTS+=ASSERTIONS=2
else
LOPTS+=--emit-symbol-map
endif
LOPTS+=-lwebsocket.js
LOPTS+=-lidbfs.js
#LOPTS+=-lglfw.js
LOPTS+=--js-library webxr/library_webxr.js

CINCS=$(addprefix -I../,$(INCS))
CFLGS=-DFT2_BUILD_LIBRARY -DDARWIN_NO_CARBON -DHAVE_UNISTD_H \
	-DOPT_GENERIC -DREAL_IS_FLOAT \
	-D_REENTRANT -DLIBXMP_CORE_PLAYER \
	-DAL_STATIC_LIBRARY -DAL_ALEXT_PROTOTYPES -DOPENAL_SUBDIR_AL -DHAVE_WEBAUDIO -DRESTRICT=__restrict \
	$(OPTS)
CFLGS+=	-fno-rtti -DEMSCRIPTEN_HAS_UNBOUND_TYPE_NAMES=0
PCFLGS+= -s SIDE_MODULE=2 -fwasm-exceptions
MCFLGS+= -s MAIN_MODULE=2 -fwasm-exceptions
#MCFLGS+= -s MAIN_MODULE=2 -s DISABLE_EXCEPTION_CATCHING=$(NO_EXCEPTIONS)

#OBJS=$(addprefix $(BUILD)/,$(addsuffix .emw.o,$(MAIN_SRCS)))

TEMPLATECOM=$(BUILD)/Html5/Html5
TEMPLATEJASM=$(BUILD)/Html5/Jasm
TEMPLATEWASM=$(BUILD)/Html5/Wasm

PLUGINS=$(HTML5_PLUGINS)
PLUGINS?=bitop luasocket json

EP_Xmp_SRCS=$(LXMP_SRCS) emscripten/ep_xmp
EP_Mp3_SRCS=$(LMP3_SRCS) emscripten/ep_mp3
EP_LIBS=EP_Xmp EP_Mp3
export HTML5_ROOT=$(PWD)

template: main

template.zip: template
	cd $(BUILD); rm -f Html5.zip; zip -r Html5.zip Html5/; 

plugins: $(addsuffix .plugin,$(PLUGINS))

plugins.clean: $(addsuffix .plugin.clean,$(PLUGINS))

%.plugin:
	R=$(PWD); cd ../plugins/$*/source/emscripten; $(MAKE); cd -

%.plugin.clean:
	R=$(PWD); cd ../plugins/$*/source/emscripten; $(MAKE) clean; cd -

SUBLIBS=CORE LBND LGID LGVFS LZIP LPNG LJPG LSUP LFRT LOAL
SUBLIBS_ARW=$(addprefix $(BUILD)/lib,$(addsuffix .w.a,$(SUBLIBS)))

define SUB_template
$$(BUILD)/lib$(1).w.a: $$(addprefix $(BUILD)/,$$(addsuffix .emw.o,$$($(1)_SRCS)))
$$(BUILD)/lib$(1).w.bc: $$(addprefix $(BUILD)/,$$(addsuffix .emw.o,$$($(1)_SRCS)))
SUBLIBS_SRCS+=$$($(1)_SRCS)
endef

define SUB_fileopt
$(1): $(2)
endef

define SUB_libopt
$(foreach src,$($(1)),$(eval $(call SUB_fileopt,$(addprefix $(BUILD)/,$(addsuffix .emw.o,$(src))),$(2))))
endef

$(call SUB_libopt,B2D_SRCS,CFLGS+=-fno-rtti -fno-exceptions)
$(call SUB_libopt,LGVFS_SRCS,CFLGS+=-DGIDEROS_LIBRARY)
$(call SUB_libopt,LGID_SRCS,CFLGS+=-DGIDEROS_LIBRARY)
$(call SUB_libopt,LOAL_SRCS,INCS+=$(addprefix libgid/external/openal-soft/,. core common alc))
 
$(foreach sub,$(SUBLIBS),$(eval $(call SUB_template,$(sub))))

SUBLIBS_WOBJS=$(addprefix $(BUILD)/,$(addsuffix .emw.o,$(SUBLIBS_SRCS)))

define EP_template
$$(BUILD)/$(1).wasm: $$(addprefix $(BUILD)/,$$(addsuffix .emwp.o,$$($(1)_SRCS)))
	@echo "EMLINK WASM" $(TARGET)
	$$(EMCC) -s SIDE_MODULE=2 -s WASM=1 -s DISABLE_EXCEPTION_CATCHING=0 $(OPTS) -o $$@ $$(addprefix $(BUILD)/,$$(addsuffix .emwp.o,$$($(1)_SRCS)))
	@echo "SYMGEN" $(TARGET)
	@$$(EMSDK_PREFIX) wasm-dis.exe $$@ | grep '(import ' | grep -v '(table ' | grep -v '(memory ' | sed 's/^[ \t]*//' | cut -d' ' -f3 >$(BUILD)/$(1).isyms
	@$$(EMSDK_PREFIX) wasm-dis.exe $$@ | grep '(export ' | sed 's/^[ \t]*//' | cut -d' ' -f2 >$(BUILD)/$(1).esyms
	@grep -Fvxf $(BUILD)/$(1).esyms $(BUILD)/$(1).isyms >$(BUILD)/$(1).syms
SUBLIBS_SRCS+=$$($(1)_SRCS)
endef

$(foreach sub,$(EP_LIBS),$(eval $(call EP_template,$(sub))))

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
COMMA :=,
EXP_FUNCS_LIST=$(sort $(shell grep -vh "invoke_XX" $(BUILD)/*.syms | sed -e 's/g\$$//;s/fp\$$/')))
EXP_FUNCS=["_main","_gapplication_clipboardCallback","__Unwind_CallPersonality","___wasm_lpad_context","setTempRet0","getTempRet0","_lua_tounsignedx",$(subst $(SPACE),$(COMMA),$(addprefix _,$(sort $(shell grep -vhE "invoke_XX|getTempRet0|setTempRet0" $(BUILD)/*.syms | sed 's/g\$$//;s/fp\$$//' |sed 's/\$$[^"]*//g'))))]

$(BUILD)/lib%.w.a:
	@echo "EMAR" $@
	@$(EMAR) r $@ $^

$(BUILD)/lib%.w.bc:
	@echo "EMLD" $@
	@$(EMCC) $(OPTS)  -o $@ -r $^

keep: 
	echo "#define KEEP(v) extern volatile const char *v; *v;" >emscripten/_keep.c
	echo "void _keep() {" >>emscripten/_keep.c
	grep -vh "invoke_" emscripten/Build/*.syms | sort | uniq | sed -e 's/g\$$//;s/"//g;s/^/KEEP\(/;s/$$/)/' >>emscripten/_keep.c
	echo "}" >>emscripten/_keep.c
					
$(BUILD)/gideros-wasm.html: $(SUBLIBS_ARW) $(addprefix $(BUILD)/,$(addsuffix .wasm,$(EP_LIBS))) $(OBJS) gidjs.js gui.js gidpost.js gidshell.html plugins
	@echo -s EXPORTED_FUNCTIONS=$(EXP_FUNCS) >$(BUILD)/exports.list
	@echo "EMLINK gideros WASM"
	$(EMCC) $(OBJS) $(LIBS) --minify 0  $(addprefix -s ,$(EOPTS) $(EOPTS_WASM)) @$(BUILD)/exports.list $(OPTS) $(LOPTS_WASM) $(SUBLIBS_ARW) -o $(BUILD)/gideros-wasm.html --pre-js gidjs.js --pre-js gui.js --post-js gidpost.js --shell-file gidshell.html
	@echo "SYMGEN gideros"
	@$(EMSDK_PREFIX) wasm-dis.exe $(BUILD)/gideros-wasm.wasm >$(BUILD)/gideros.wdis
	@$(EMSDK_PREFIX) wasm-dis.exe $(BUILD)/gideros-wasm.wasm | sed 's/^[ \t]*//' | cut -d' ' -f2 >$(BUILD)/gideros.esyms
	

main: path $(BUILD)/gideros-wasm.html 
	cp gideros.png $(TEMPLATECOM)
	cp gideros.css $(TEMPLATECOM)
	cp manifest.json serviceWorker.js $(TEMPLATECOM)
	cp $(BUILD)/gideros-wasm.wasm $(TEMPLATEWASM) 
	cp $(BUILD)/gideros-wasm.js $(TEMPLATEWASM) 
	cp $(BUILD)/gideros-wasm.html $(TEMPLATEWASM)/index.html
	-cp $(BUILD)/gideros-wasm.html.symbols $(TEMPLATEWASM)
	cp $(BUILD)/EP_*.wasm $(TEMPLATEWASM)
	cp gidloader.js $(TEMPLATEWASM)
	#cp lzma.js $(TEMPLATECOM)
	#$(CRUNCHME_TOOL) -nostrip -i pace.min.js $(TEMPLATE)/pace.js.png
	#$(CRUNCHME_TOOL) -nostrip -i $(BUILD)/gideros.js $(TEMPLATE)/gideros.js.png
	

path:
	mkdir -p $(sort $(dir $(SUBLIBS_WOBJS)))
	mkdir -p $(TEMPLATECOM) $(TEMPLATEWASM)

clean:
	rm -rf $(BUILD)

setup:
	$(EMSDK_PREFIX) emsdk.bat activate latest
	
init:
	$(EMSDK_PREFIX) emcc

$(BUILD)/%.emw.o: ../%.cpp
	@echo "EMWC+ $<"
	@$(EMCC) $(CINCS) $(CFLGS) $(MCFLGS) -std=c++17 -c $< -o $@

$(BUILD)/%.emw.o: ../%.c
	@echo "EMWCC $<"
	@$(EMCC) $(CINCS) $(CFLGS) $(MCFLGS) -c $< -o $@

$(BUILD)/%.emwp.o: ../%.cpp
	@echo "EMWC+ $<"
	@$(EMCC) $(CINCS) $(CFLGS) $(PCFLGS) -std=c++17 -c $< -o $@

$(BUILD)/%.emwp.o: ../%.c
	@echo "EMWCC $<"
	@$(EMCC) $(CINCS) $(CFLGS) $(PCFLGS) -c $< -o $@
