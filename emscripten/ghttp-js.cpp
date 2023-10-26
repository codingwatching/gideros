#include <ghttp.h>
#include <stdio.h>
#include <map>
#include <string>
#include "emscripten.h"

struct GHttpContext
{
/*  GHttpContex(const GHttpContext &c)
  {
	  id=c.id;
	  callback=c.callback;
	  udata=c.udata;
	  xhrId=c.xhrId;
  }*/
  g_id id;
  gevent_Callback callback;
  void *udata;
  int xhrId;
};

static std::map<g_id, GHttpContext> map_;

extern "C" {

void ghttp_onload(int xh,int arg,const char *buf,int size, int status, char *hdrs,int header)
{
	//printf("OnLoad:%d,%d,%p,%d,%s\n",xh,arg,buf,size,hdrs);
	std::map<g_id, GHttpContext>::iterator it=map_.find(arg);
	if (it!=map_.end())
	{
		struct GHttpContext ctx=it->second;		
		map_.erase(it);
		
        std::map<std::string,std::string> headers;
        while (true)
        {
         char *le=strstr(hdrs,"\r\n");
         if (!le) break;
         char *eq=strstr(hdrs,": ");
         if (!eq) break;
         *le=0;
         *eq=0;
         std::string kh=hdrs;
         std::string vh=eq+2;
         hdrs=le+2;
         headers[kh]=vh;
        }
        
        int hdrCount=headers.size();
        int hdrSize=0;
        std::map<std::string,std::string>::iterator h;
        for (h = headers.begin(); h != headers.end(); ++h)
        {
        	hdrSize+=h->first.size();
           	hdrSize+=h->second.size();
           	hdrSize+=2;
        }

        ghttp_ResponseEvent *event = (ghttp_ResponseEvent*)malloc(sizeof(ghttp_ResponseEvent)  + sizeof(ghttp_Header)*hdrCount + size + hdrSize);

        event->data = (char*)event + sizeof(ghttp_ResponseEvent) + sizeof(ghttp_Header)*hdrCount;
        if (buf)
        	memcpy(event->data, buf,size);
        event->size = size;

        event->httpStatusCode = status;

		int hdrn=0;
		char *hdrData=(char *)(event->data)+size;
        for (h = headers.begin(); h != headers.end(); ++h)
        {
        	int ds=h->first.size();
        	memcpy(hdrData,h->first.c_str(),ds);
	 		event->headers[hdrn].name=hdrData;
        	hdrData+=ds;
        	*(hdrData++)=0;
        	ds=h->second.size();
        	memcpy(hdrData,h->second.c_str(),ds);
	 		event->headers[hdrn].value=hdrData;
        	hdrData+=ds;
        	*(hdrData++)=0;
			hdrn++;
        }
		event->headers[hdrn].name=NULL;
		event->headers[hdrn].value=NULL;

        gevent_EnqueueEvent(ctx.id, ctx.callback, header?GHTTP_HEADER_EVENT:GHTTP_RESPONSE_EVENT, event, 1, ctx.udata);

	}
}

void ghttp_onprogress(int xh,int arg,int pg,int tot,const char *buf,int size)
{
	//printf("OnProgress:%d,%d,%d,%d\n",xh,arg,pg,tot);
	std::map<g_id, GHttpContext>::iterator it=map_.find(arg);
	if (it!=map_.end())
	{
		struct GHttpContext ctx=it->second;
		ghttp_ProgressEvent* event = (ghttp_ProgressEvent*)malloc(sizeof(ghttp_ProgressEvent)+size);
		event->bytesLoaded = pg;
		event->bytesTotal = tot;
		if (buf) {
			event->chunk=(void *)(event+1);
        	memcpy(event->chunk, buf,size);
		}
		else
			event->chunk=NULL;
		event->chunkSize=size;
            
        gevent_EnqueueEvent(ctx.id, ctx.callback, GHTTP_PROGRESS_EVENT, event, 1, ctx.udata);
	}	
}

void ghttp_onerror(int xh,int arg,int sts,const char *text)
{
	//printf("OnError:%d,%d,%d,%s\n",xh,arg,sts,text);
	std::map<g_id, GHttpContext>::iterator it=map_.find(arg);
	if (it!=map_.end())
	{
		struct GHttpContext ctx=it->second;
		ghttp_ErrorEvent *event = (ghttp_ErrorEvent*)gevent_CreateEventStruct1(
			                                           sizeof(ghttp_ErrorEvent),
			                                        offsetof(ghttp_ErrorEvent, error), text);
        
        gevent_EnqueueEvent(ctx.id, ctx.callback, GHTTP_ERROR_EVENT, event, 1, ctx.udata);
		map_.erase(it);
	}	
}

void ghttp_Init()
{
}

void ghttp_Cleanup()
{
	ghttp_CloseAll();
}

g_id ghttp_Get(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
	struct GHttpContext ctx;
    ctx.id = g_NextId();
    ctx.callback = callback;
    ctx.udata = udata;
    ctx.xhrId=0;
    map_[ctx.id] = ctx;
    
   ctx.xhrId=EM_ASM_INT({
        	return ($6?Module.ghttpjs_urlstream:Module.ghttpjs_urlload)(Module.UTF8ToString($0),'GET',$1,null,$2,true,$3,$4,$5);
        },url,header,ctx.id,(int)ghttp_onload, (int)ghttp_onerror, (int)ghttp_onprogress,streaming);
    //printf("GET:%ld/%d %s\n",ctx.id,ctx.xhrId,url);

    return ctx.id;
}

g_id ghttp_Post(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
	struct GHttpContext ctx;
    ctx.id = g_NextId();
    ctx.callback = callback;
    ctx.udata = udata;
    ctx.xhrId=0;
    map_[ctx.id] = ctx;

	ctx.xhrId=EM_ASM_INT({
			return ($8?Module.ghttpjs_urlstream:Module.ghttpjs_urlload)(Module.UTF8ToString($0),'POST',$1,Module.HEAPU8.subarray($6,$6+$7),$2,true,$3,$4,$5);
		},url,header,ctx.id,(int)ghttp_onload, (int)ghttp_onerror, (int)ghttp_onprogress,data,size,streaming);
    //printf("POST:%ld/%d %s\n",ctx.id,ctx.xhrId,url);

    return ctx.id;
}

g_id ghttp_Delete(const char* url, const ghttp_Header *header, int streaming, gevent_Callback callback, void* udata)
{
	struct GHttpContext ctx;
    ctx.id = g_NextId();
    ctx.callback = callback;
    ctx.udata = udata;
    ctx.xhrId=0;
    map_[ctx.id] = ctx;

   ctx.xhrId=EM_ASM_INT({
        	return ($6?Module.ghttpjs_urlstream:Module.ghttpjs_urlload)(Module.UTF8ToString($0),'DELETE',$1,null,$2,true,$3,$4,$5);
        },url,header,ctx.id,(int)ghttp_onload, (int)ghttp_onerror, (int)ghttp_onprogress,streaming);
    //printf("GET:%ld/%d %s\n",ctx.id,ctx.xhrId,url);

    return ctx.id;
}

g_id ghttp_Put(const char* url, const ghttp_Header *header, const void* data, size_t size, int streaming, gevent_Callback callback, void* udata)
{
	struct GHttpContext ctx;
    ctx.id = g_NextId();
    ctx.callback = callback;
    ctx.udata = udata;
    ctx.xhrId=0;
    map_[ctx.id] = ctx;

	ctx.xhrId=EM_ASM_INT({
			return ($8?Module.ghttpjs_urlstream:Module.ghttpjs_urlload)(Module.UTF8ToString($0),'PUT',$1,Module.HEAPU8.subarray($6,$6+$7),$2,true,$3,$4,$5);
		},url,header,ctx.id,(int)ghttp_onload, (int)ghttp_onerror, (int)ghttp_onprogress,data,size,streaming);

    return ctx.id;
}

void ghttp_Close(g_id id)
{
	std::map<g_id, GHttpContext>::iterator it=map_.find(id);
	if (it!=map_.end())
	{
		struct GHttpContext ctx=it->second;
		//TODO Close
		map_.erase(it);
	}
}

void ghttp_CloseAll()
{
    while (!map_.empty())
        ghttp_Close(map_.begin()->second.id);
}

void ghttp_IgnoreSSLErrors()
{
}

void ghttp_SetProxy(const char *host, int port, const char *user, const char *pass)
{
}


}
