#ifndef _GINPUT_PI_H_
#define _GINPUT_PI_H_

#include <gglobal.h>

#ifdef __cplusplus
extern "C" {
#endif

G_API void ginputp_mouseDown(int x, int y, int button, int mod);
G_API void ginputp_mouseMove(int x, int y, int mod);
G_API void ginputp_mouseUp(int x, int y, int button, int mod);
G_API void ginputp_mouseWheel(int x, int y, int button, int delta, int mod);
G_API void ginputp_keyDown(int keyCode);
G_API void ginputp_keyUp(int keyCode);
G_API void ginputp_touchBegin(int x, int y, int id, int mod);
G_API void ginputp_touchMove(int x, int y, int id, int mod);
G_API void ginputp_touchEnd(int x, int y, int id, int mod);
G_API void ginputp_touchCancel(int x, int y, int id, int mod);

#ifdef __cplusplus
}
#endif

#endif
