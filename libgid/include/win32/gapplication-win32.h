#ifndef GAPPLICATION_WIN32_H
#define GAPPLICATION_WIN32_H

#include <gglobal.h>
#include <gevent.h>

#ifdef __cplusplus
#include <windows.h>
struct W32FullScreen {
	bool maximized;
	bool isFullScreen;
	LONG style;
	LONG ex_style;
	RECT window_rect;
};

void W32SetFullScreen(bool fullScreen,HWND wnd,W32FullScreen *save);

extern "C" {
#endif
G_API void gapplication_init_w32(HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif
