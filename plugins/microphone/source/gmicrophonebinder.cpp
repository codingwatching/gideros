#include "gideros.h"
#include "lua.h"
#include "lauxlib.h"

#undef interface
#include <gaudio.h>
#include <ggaudiomanager.h>
#include "gmicrophone.h"
#include "gsoundencoder.h"

/*
  local microphone = Microphone.new(nil, 44100, 1, 16)
  microphone:setOutputFile("|D|hebe.wav")
  microphone:start()
  -- after some time
  microphone:stop()
*/

static lua_State *L = NULL;

namespace {

static const char* DATA_AVAILABLE = "dataAvailable";

// some Lua helper functions
#ifndef abs_index
#define abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)
#endif

static void luaL_newweaktable(lua_State *L, const char *mode)
{
    lua_newtable(L);			// create table for instance list
    lua_pushstring(L, mode);
    lua_setfield(L, -2, "__mode");	  // set as weak-value table
    lua_pushvalue(L, -1);             // duplicate table
    lua_setmetatable(L, -2);          // set itself as metatable
}

static void luaL_rawgetptr(lua_State *L, int idx, void *ptr)
{
    idx = abs_index(L, idx);
    lua_pushlightuserdata(L, ptr);
    lua_rawget(L, idx);
}

static void luaL_rawsetptr(lua_State *L, int idx, void *ptr)
{
    idx = abs_index(L, idx);
    lua_pushlightuserdata(L, ptr);
    lua_insert(L, -2);
    lua_rawset(L, idx);
}

static char keyStrong = ' ';
static char keyWeak = ' ';

class GMicrophone : public GEventDispatcherProxy
{
public:
    GMicrophone(lua_State *L, bool noMic, const char *deviceName, int numChannels, int sampleRate, int bitsPerSample, float quality, gmicrophone_Error *error)
    {
        if (++instanceCount_ == 1)
            gmicrophone_Init();

        outputFile_ = 0;

        if (!noMic) {
            microphone_ = gmicrophone_Create(deviceName, numChannels, sampleRate, bitsPerSample, error);

            if (microphone_ == 0)
                return;

            gmicrophone_AddCallback(microphone_, callback_s, this);
        }
        else
            microphone_ = 0;


        numChannels_ = numChannels;
        sampleRate_ = sampleRate;
        bitsPerSample_ = bitsPerSample;
    	bytesPerSample_=((bitsPerSample + 7) / 8) * numChannels;
    	quality_=quality;
        encoder=NULL;

        started_ = false;
        paused_ = false;
    }

    ~GMicrophone()
    {
        if (microphone_)
            gmicrophone_Delete(microphone_);

        if (outputFile_&&encoder)
            encoder->close(outputFile_);

        if (--instanceCount_ == 0)
            gmicrophone_Cleanup();
    }

    void start(std::string* error)
    {
        if (started_)
            return;

        if (!fileName_.empty())
        {
        	encoder=gaudio_lookupEncoder(fileName_.c_str());
        	if (encoder)
        		outputFile_ = encoder->open(fileName_.c_str(), numChannels_, sampleRate_, bitsPerSample_,quality_);

            if (outputFile_ == 0)
            {
                if (error)
                    *error = fileName_ + ": Cannot create output file.";
                return;
            }
        }

        gmicrophone_Start(microphone_);

        started_ = true;
    }

    void stop()
    {
        if (!started_)
            return;

        gmicrophone_Stop(microphone_);

        if (outputFile_)
        {
        	if (encoder)
        		encoder->close(outputFile_);
            encoder = NULL;
            outputFile_ = 0;
        }

        started_ = false;
    }

    void setPaused(bool paused)
    {
        if (!started_)
            return;

        if (paused_ == paused)
            return;

        if (paused)
            gmicrophone_Stop(microphone_);
        else
            gmicrophone_Start(microphone_);

        paused_ = paused;
    }

    bool isPaused() const
    {
        return paused_;
    }

    bool isRecording() const
    {
        return started_ && !paused_;
    }

    int getStreamId() const
    {
        return outputFile_;
    }

    bool isStarted() const
    {
        return started_;
    }

    void setOutputFile(const char *fileName)
    {
        if (started_)
            return;

        fileName_ = fileName;
    }

    void clearOutputFile()
    {
        if (started_)
            return;

        fileName_.clear();
    }

    size_t writeAudio(void *buf,size_t blen)
    {
        if (outputFile_&&encoder)
            return encoder->write(outputFile_,blen,buf);
        return 0;
    }

private:
    static void callback_s(int type, void *event, void *udata)
    {
        static_cast<GMicrophone*>(udata)->callback(type, event);
    }

    void callback(int type, void *event)
    {
        if (type == GMICROPHONE_DATA_AVAILABLE_EVENT)
        {
            gmicrophone_DataAvailableEvent* event2 = (gmicrophone_DataAvailableEvent*)event;
            if (outputFile_&&encoder)
                encoder->write(outputFile_,event2->sampleCount * bytesPerSample_, event2->data);


            luaL_rawgetptr(L, LUA_REGISTRYINDEX, &keyWeak);
            luaL_rawgetptr(L, -1, this);

            if (lua_isnil(L, -1))
            {
                lua_pop(L, 2);
                return;
            }

            lua_getfield(L, -1, "dispatchEvent");

            lua_pushvalue(L, -2);

            lua_getfield(L, -1, "__dataAvailableEvent");

            lua_pushnumber(L, event2->averageAmplitude);
            lua_setfield(L, -2, "averageAmplitude");

            lua_pushnumber(L, event2->peakAmplitude);
            lua_setfield(L, -2, "peakAmplitude");

            lua_call(L, 2, 0);

            lua_pop(L, 2);
        }
    }

private:
    g_id microphone_;
    int numChannels_, sampleRate_, bitsPerSample_, bytesPerSample_;
    float quality_;
    g_id outputFile_;
    bool started_;
    bool paused_;
    std::string fileName_;
    GGAudioEncoder *encoder;
    static int instanceCount_;
};

int GMicrophone::instanceCount_ = 0;


static int getDeviceList(lua_State *L)
{
    std::vector<std::string> list;
    gmicrophone_GetDeviceList(list);
    int ls=list.size();
    lua_createtable(L,ls,0);
    for (int k=0;k<ls;k++)
    {
        lua_pushstring(L,list[k].c_str());
        lua_rawseti(L,-2,k+1);
    }
    return 1;
}

static int create(lua_State *L)
{
    const char *devName=NULL;
    bool noMic=false;
    int argIdx=2;
    if (lua_isnumber(L,1)) //No device name at all, not even nil: assume no mic
    {
        noMic=true;
        argIdx=1;
    }
    else
        devName=luaL_optstring(L,1,NULL);
    int sampleRate = (int)luaL_checkinteger(L, argIdx);
    int numChannels = (int)luaL_checkinteger(L, argIdx+1);
    int bitsPerSample = (int)luaL_optinteger(L, argIdx+2,16);
    float quality = (float) luaL_optnumber(L, argIdx+3,0.5); //Balanced

    gmicrophone_Error error=GMICROPHONE_NO_ERROR;
    GMicrophone *microphone = new GMicrophone(L, noMic, devName, numChannels, sampleRate, bitsPerSample, quality, &error);

    switch (error)
    {
    case GMICROPHONE_NO_ERROR:
        break;
    case GMICROPHONE_CANNOT_OPEN_DEVICE:
        delete microphone;
        lua_pushnil(L);
        return 1;
    case GMICROPHONE_UNSUPPORTED_FORMAT:
        delete microphone;
        luaL_error(L, "Unsupported microphone format.");
    case GMICROPHONE_PROMPTING_PERMISSION:
        delete microphone;
        luaL_error(L, "Permission requested.");
    }

    g_pushInstance(L, "Microphone", microphone->object());

    if (!noMic) {
        lua_getglobal(L, "Event");
        lua_getfield(L, -1, "new");
        lua_remove(L, -2);

        lua_pushstring(L, DATA_AVAILABLE);
        lua_call(L, 1, 1);

        lua_setfield(L, -2, "__dataAvailableEvent");


        luaL_rawgetptr(L, LUA_REGISTRYINDEX, &keyWeak);
        lua_pushvalue(L, -2);
        luaL_rawsetptr(L, -2, microphone);
        lua_pop(L, 1);
    }

    return 1;
}

static int destruct(void *p)
{
    void* ptr = GIDEROS_DTOR_UDATA(p);
    GReferenced* object = static_cast<GReferenced*>(ptr);
    GReferenced* proxy = object->proxy();
    proxy->unref();

    return 0;
}

static GMicrophone *getInstance(lua_State *L, int index)
{
    GReferenced *object = static_cast<GReferenced*>(g_getInstance(L, "Microphone", index));
    GReferenced *proxy = object->proxy();

    return static_cast<GMicrophone*>(proxy);
}

static int start(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);

    std::string error;
    microphone->start(&error);
    if (!error.empty())
        luaL_error(L, "%s", error.c_str());

    luaL_rawgetptr(L, LUA_REGISTRYINDEX, &keyStrong);
    lua_pushvalue(L, -2);
    luaL_rawsetptr(L, -2, microphone);
    lua_pop(L, 1);

    return 0;
}

static int stop(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    microphone->stop();

    luaL_rawgetptr(L, LUA_REGISTRYINDEX, &keyStrong);
    lua_pushnil(L);
    luaL_rawsetptr(L, -2, microphone);
    lua_pop(L, 1);

    return 0;
}

static int lua_toboolean2(lua_State *L, int idx)
{
    if (lua_isnone(L, idx))
        luaL_typerror(L, idx, "boolean");

    return lua_toboolean(L, idx);
}

static int setPaused(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    bool paused = lua_toboolean2(L, 2);
    microphone->setPaused(paused);

    return 0;
}

static int setOutputFile(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    const char *fileName = luaL_checkstring(L, 2);

    if (microphone->isStarted())
        luaL_error(L, "Cannot set output file while recording.");

    microphone->setOutputFile(fileName);

    return 0;
}

static int isPaused(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    lua_pushboolean(L, microphone->isPaused());
    return 1;
}

static int isRecording(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    lua_pushboolean(L, microphone->isRecording());
    return 1;
}

static int getStreamId(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    lua_pushinteger(L, microphone->getStreamId());
    return 1;
}

static int writeAudio(lua_State *L)
{
    GMicrophone *microphone = getInstance(L, 1);
    void *buf;
    size_t blen;
    if ((buf=lua_tobuffer(L,2,&blen))==NULL)
        buf=(void *)luaL_checklstring(L,2,&blen);

    if (!microphone->isStarted())
        luaL_error(L, "Recording not started.");

    lua_pushunsigned(L,microphone->writeAudio(buf,blen));

    return 1;
}


static int loader(lua_State* L)
{
    const luaL_Reg functionlist[] = {
        {"start", start},
        {"stop", stop},
        {"setPaused", setPaused},
        {"isPaused", isPaused},
        {"isRecording", isRecording},
        {"setOutputFile", setOutputFile},
        {"getStreamId", getStreamId},
        {"writeAudio", writeAudio},
        {"getDeviceList", getDeviceList},
        {NULL, NULL},
    };

    g_createClass(L, "Microphone", "EventDispatcher", create, destruct, functionlist);

    lua_getglobal(L, "Event");
    lua_pushstring(L, DATA_AVAILABLE);
    lua_setfield(L, -2, "DATA_AVAILABLE");
    lua_pop(L, 1);

    lua_newtable(L);
    luaL_rawsetptr(L, LUA_REGISTRYINDEX, &keyStrong);

    luaL_newweaktable(L, "v");
    luaL_rawsetptr(L, LUA_REGISTRYINDEX, &keyWeak);

    lua_getglobal(L, "Microphone");
    return 1;
}

}

GGAudioEncoder audioWav(gsoundencoder_WavCreate, gsoundencoder_WavClose, gsoundencoder_WavWrite);

static void g_initializePlugin(lua_State *L)
{
	::L = L;
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    lua_pushcnfunction(L, loader,"plugin_init_microphone");
    lua_setfield(L, -2, "microphone");

    lua_pop(L, 2);
    gaudio_registerEncoderType("wav",audioWav);
}

static void g_deinitializePlugin(lua_State *L)
{
	::L = NULL;
}

#if (!defined(QT_NO_DEBUG)) && (defined(TARGET_OS_MAC) || defined(_MSC_VER)  || defined(TARGET_OS_OSX))
REGISTER_PLUGIN_STATICNAMED_CPP("Microphone", "1.0",Microphone)
#else
REGISTER_PLUGIN("Microphone", "1.0")
#endif
