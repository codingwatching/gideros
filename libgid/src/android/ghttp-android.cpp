#include <ghttp.h>
#include <jni.h>
#include <map>

extern "C" {
JavaVM *g_getJavaVM();
JNIEnv *g_getJNIEnv();
}

static jobjectArray toJava(JNIEnv *env, const ghttp_Header *headers)
{
	int headerCount = 0;
	const ghttp_Header *headers2 = headers;
	for (; headers2 && headers2->name; ++headers2)
		headerCount++;
		
	if (headerCount == 0)
		return NULL;
	
	jobjectArray jheaders = (jobjectArray)env->NewObjectArray(headerCount * 2,
											env->FindClass("java/lang/String"),
											env->NewStringUTF(""));

	for (int i = 0; headers && headers->name; ++headers, ++i)
	{
		jstring jname = env->NewStringUTF(headers->name);
		jstring jvalue = env->NewStringUTF(headers->value);
		
		env->SetObjectArrayElement(jheaders, i * 2, jname);
		env->SetObjectArrayElement(jheaders, i * 2 + 1, jvalue);
		
		env->DeleteLocalRef(jname);
		env->DeleteLocalRef(jvalue);
	}
	
	return jheaders;
}

class HTTPManager
{
public:
	static HTTPManager* s_manager;
	HTTPManager()
	{
		JNIEnv *env = g_getJNIEnv();

		jclass localRefCls = env->FindClass("com/giderosmobile/android/player/HTTPManager");
		javaNativeBridge_ = (jclass)env->NewGlobalRef(localRefCls);
		env->DeleteLocalRef(localRefCls);

		initId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Init", "()V");
		cleanupId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Cleanup", "()V");
		getId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Get", "(Ljava/lang/String;[Ljava/lang/String;ZJJ)V");
		postId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Post", "(Ljava/lang/String;[Ljava/lang/String;[BZJJ)V");
		putId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Put", "(Ljava/lang/String;[Ljava/lang/String;[BZJJ)V");
		deleteId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Delete", "(Ljava/lang/String;[Ljava/lang/String;ZJJ)V");
		closeId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_Close", "(J)V");
		closeAllId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_CloseAll", "()V");
		ignoreSslErrorsId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_IgnoreSslErrors", "()V");
		setProxyId_ = env->GetStaticMethodID(javaNativeBridge_, "ghttp_SetProxy", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)V");

		env->CallStaticVoidMethod(javaNativeBridge_, initId_);
	}
	
	~HTTPManager()
	{
		JNIEnv *env = g_getJNIEnv();
		env->CallStaticVoidMethod(javaNativeBridge_, cleanupId_);
		env->DeleteGlobalRef(javaNativeBridge_);
	}
	
	void IgnoreSslErrors()
	{
		JNIEnv *env = g_getJNIEnv();
		env->CallStaticVoidMethod(javaNativeBridge_, ignoreSslErrorsId_);
	}

	void SetProxy(const char *host,int port,const char *user,const char *pass)
	{
		JNIEnv *env = g_getJNIEnv();

		jstring jhost = env->NewStringUTF(host);
		jstring juser = env->NewStringUTF(user);
		jstring jpass = env->NewStringUTF(pass);

		env->CallStaticVoidMethod(javaNativeBridge_, setProxyId_, jhost, (jint)port, juser, jpass);

		env->DeleteLocalRef(jhost);
		env->DeleteLocalRef(juser);
		env->DeleteLocalRef(jpass);
	}

	g_id Get(const char *url, const ghttp_Header *headers, bool streaming, gevent_Callback callback, void *udata)
	{
		JNIEnv *env = g_getJNIEnv();

		jstring jurl = env->NewStringUTF(url);

		jobjectArray jheaders = toJava(env, headers);
		
		g_id id = g_NextId();
		
		env->CallStaticVoidMethod(javaNativeBridge_, getId_, jurl, jheaders, (jboolean)streaming, (jlong)this, (jlong)id);

		if (jheaders)
			env->DeleteLocalRef(jheaders);
		
		env->DeleteLocalRef(jurl);

		CallbackElement element;
		element.callback = callback;
		element.udata = udata;
		map_[id] = element;
		
		return id;
	}

	g_id Post(const char *url, const ghttp_Header *headers, const void *data, size_t size, bool streaming, gevent_Callback callback, void *udata)
	{
		JNIEnv *env = g_getJNIEnv();

		jstring jurl = env->NewStringUTF(url);

		jobjectArray jheaders = toJava(env, headers);

		jbyteArray jdata = NULL;
		if (size > 0)
		{
			jdata = env->NewByteArray(size);
			env->SetByteArrayRegion(jdata, 0, size, (jbyte*)data);
		}

		g_id id = g_NextId();

		env->CallStaticVoidMethod(javaNativeBridge_, postId_, jurl, jheaders, jdata, (jboolean)streaming, (jlong)this, (jlong)id);

		if (jdata)
			env->DeleteLocalRef(jdata);

		if (jheaders)
			env->DeleteLocalRef(jheaders);

		env->DeleteLocalRef(jurl);

		CallbackElement element;
		element.callback = callback;
		element.udata = udata;
		map_[id] = element;
		
		return id;
	}

	g_id Put(const char *url, const ghttp_Header *headers, const void *data, size_t size, bool streaming, gevent_Callback callback, void *udata)
	{
		JNIEnv *env = g_getJNIEnv();

		jstring jurl = env->NewStringUTF(url);

		jobjectArray jheaders = toJava(env, headers);

		jbyteArray jdata = NULL;
		if (size > 0)
		{
			jdata = env->NewByteArray(size);
			env->SetByteArrayRegion(jdata, 0, size, (jbyte*)data);
		}

		g_id id = g_NextId();

		env->CallStaticVoidMethod(javaNativeBridge_, putId_, jurl, jheaders, jdata, (jboolean)streaming, (jlong)this, (jlong)id);

		if (jdata)
			env->DeleteLocalRef(jdata);

		if (jheaders)
			env->DeleteLocalRef(jheaders);

		env->DeleteLocalRef(jurl);

		CallbackElement element;
		element.callback = callback;
		element.udata = udata;
		map_[id] = element;
		
		return id;
	}
	
	g_id Delete(const char *url, const ghttp_Header *headers, bool streaming, gevent_Callback callback, void *udata)
	{
		JNIEnv* env = g_getJNIEnv();

		jstring jurl = env->NewStringUTF(url);

		jobjectArray jheaders = toJava(env, headers);

		g_id id = g_NextId();

		env->CallStaticVoidMethod(javaNativeBridge_, deleteId_, jurl, jheaders, (jboolean)streaming, (jlong)this, (jlong)id);

		if (jheaders)
			env->DeleteLocalRef(jheaders);

		env->DeleteLocalRef(jurl);

		CallbackElement element;
		element.callback = callback;
		element.udata = udata;
		map_[id] = element;
		
		return id;
	}
		
	void Close(g_id id)
	{
		JNIEnv *env = g_getJNIEnv();
		env->CallStaticVoidMethod(javaNativeBridge_, closeId_, (jlong)id);
	
		map_.erase(id);
	}

	void CloseAll()
	{
		JNIEnv *env = g_getJNIEnv();
		env->CallStaticVoidMethod(javaNativeBridge_, closeAllId_);
		
		map_.clear();	
	}
		
	struct CallbackElement
	{
		gevent_Callback callback;
		void *udata;	
	};
	
	// This callback intercept HTTP finish events, calls real handler, then remove this request from the map
	void callback(int type, void *event, void *udata)
	{
		g_id id=(g_id)udata;
		CallbackElement &element = map_[id];
		if (element.callback)
			element.callback(type,event,element.udata);
		if (type!=GHTTP_HEADER_EVENT)
			map_.erase(id);
	}

	static void callback_s(int type, void *event, void *udata)
	{
		if (s_manager)
			s_manager->callback(type,event,udata);
	}

	void ghttp_responseCallback(JNIEnv *env, jlong id, jbyteArray jdata, jint size, jint statusCode, jint hdrCount, jint hdrSize, jboolean header)
	{
		if (map_.find(id) == map_.end())
			return;

		CallbackElement &element = map_[id];
			
		jbyte *data = jdata?(jbyte*)env->GetPrimitiveArrayCritical(jdata, 0):NULL;
		
		ghttp_ResponseEvent *event = (ghttp_ResponseEvent*)malloc(sizeof(ghttp_ResponseEvent) + sizeof(ghttp_Header)*hdrCount + size + hdrSize);

		event->data = (char*)event + sizeof(ghttp_ResponseEvent) + sizeof(ghttp_Header)*hdrCount;
		memcpy(event->data, data, size + hdrSize);
		event->size = size;
		event->httpStatusCode = statusCode;

		int hdrn=0;
		char *hdrData=(char *)(event->data)+size;
		while (hdrCount--)
		{
	 		 event->headers[hdrn].name=hdrData;
			 hdrData+=(strlen(hdrData)+1);
	 		 event->headers[hdrn].value=hdrData;
			 hdrData+=(strlen(hdrData)+1);
			 hdrn++;
		}
		event->headers[hdrn].name=NULL;
		event->headers[hdrn].value=NULL;

		gevent_EnqueueEvent(id, callback_s, header?GHTTP_HEADER_EVENT:GHTTP_RESPONSE_EVENT, event, 1, (void *)id);

		if (jdata)
			env->ReleasePrimitiveArrayCritical(jdata, data, 0);
	}
	
	void ghttp_errorCallback(JNIEnv *env, jlong id, jstring error)
	{
		if (map_.find(id) == map_.end())
			return;

		CallbackElement &element = map_[id];

		jboolean isCopy;
		const char *cerror = env->GetStringUTFChars(error, &isCopy);

		ghttp_ErrorEvent *event = (ghttp_ErrorEvent*)gevent_CreateEventStruct1(
			                                           sizeof(ghttp_ErrorEvent),
			                                        offsetof(ghttp_ErrorEvent, error), cerror);

        gevent_EnqueueEvent(id, callback_s, GHTTP_ERROR_EVENT, event, 1, (void *)id);
	}
	
	void ghttp_progressCallback(JNIEnv *env, jlong id, jint bytesLoaded, jint bytesTotal, jbyteArray jdata, jint size)
	{
		if (map_.find(id) == map_.end())
			return;

		CallbackElement &element = map_[id];

		ghttp_ProgressEvent *event = (ghttp_ProgressEvent*)malloc(sizeof(ghttp_ProgressEvent)+size);
		event->bytesLoaded = bytesLoaded;
		event->bytesTotal = bytesTotal;
		if (jdata) {
			event->chunk=(void *)(event+1);
			jbyte *data = (jbyte*)env->GetPrimitiveArrayCritical(jdata, 0);
        	memcpy(event->chunk, data,size);
    		env->ReleasePrimitiveArrayCritical(jdata, data, 0);
		}
		else
			event->chunk=NULL;
		event->chunkSize=size;

		gevent_EnqueueEvent(id, element.callback, GHTTP_PROGRESS_EVENT, event, 1, element.udata);
	}
	
private:
	jclass javaNativeBridge_;

	jmethodID initId_;
	jmethodID cleanupId_;
	jmethodID getId_;
	jmethodID postId_;
	jmethodID putId_;
	jmethodID deleteId_;
	jmethodID closeId_;
	jmethodID closeAllId_;
	jmethodID ignoreSslErrorsId_;
	jmethodID setProxyId_;

	std::map<g_id, CallbackElement> map_;
};

extern "C" {

void Java_com_giderosmobile_android_player_HTTPManager_nativeghttpResponseCallback(JNIEnv *env, jclass cls, jlong id, jbyteArray data, jint size, jint statusCode, jint hdrCount, jint hdrSize, jboolean header, jlong udata)
{
	HTTPManager *that = (HTTPManager*)udata;
	that->ghttp_responseCallback(env, id, data, size, statusCode, hdrCount, hdrSize, header);
}

void Java_com_giderosmobile_android_player_HTTPManager_nativeghttpErrorCallback(JNIEnv *env, jclass cls, jlong id, jlong udata, jstring error)
{
	HTTPManager *that = (HTTPManager*)udata;
	that->ghttp_errorCallback(env, id, error);
}

void Java_com_giderosmobile_android_player_HTTPManager_nativeghttpProgressCallback(JNIEnv *env, jclass cls, jlong id, jint bytesLoaded, jint bytesTotal, jbyteArray data, jint size, jlong udata)
{
	HTTPManager *that = (HTTPManager*)udata;
	that->ghttp_progressCallback(env, id, bytesLoaded, bytesTotal, data, size);
}

}

HTTPManager* HTTPManager::s_manager = NULL;

extern "C" {
void ghttp_IgnoreSSLErrors()
{
	HTTPManager::s_manager->IgnoreSslErrors();
}

void ghttp_SetProxy(const char *host, int port, const char *user, const char *pass)
{
	HTTPManager::s_manager->SetProxy(host, port, user, pass);
}

void ghttp_Init()
{
	HTTPManager::s_manager = new HTTPManager();
}

void ghttp_Cleanup()
{
	delete HTTPManager::s_manager;
	HTTPManager::s_manager = NULL;
}

g_id ghttp_Get(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
	return HTTPManager::s_manager->Get(url, header, streaming, callback, udata);
}

g_id ghttp_Post(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
	return HTTPManager::s_manager->Post(url, header, data, size, streaming, callback, udata);
}

g_id ghttp_Delete(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
	return HTTPManager::s_manager->Delete(url, header, streaming, callback, udata);
}

g_id ghttp_Put(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
	return HTTPManager::s_manager->Put(url, header, data, size, streaming, callback, udata);
}

void ghttp_Close(g_id id)
{
	HTTPManager::s_manager->Close(id);
}

void ghttp_CloseAll()
{
	HTTPManager::s_manager->CloseAll();
}

}

