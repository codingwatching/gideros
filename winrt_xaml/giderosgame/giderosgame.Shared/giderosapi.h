#ifndef GIDEROSAPI_H
#define GIDEROSAPI_H

using namespace Windows::UI::Core;
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

	void gdr_initialize(bool useXaml, CoreWindow^ Window, Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanel,
						int width, int height, bool player, const wchar_t* resourcePath, const wchar_t* docsPath, const wchar_t* tempPath);
	void gdr_drawFirstFrame();
	void gdr_drawFrame(bool useXaml);
	void gdr_exitGameLoop();
	void gdr_deinitialize();
	void gdr_suspend();
	void gdr_resume();
	void gdr_didReceiveMemoryWarning();
	void gdr_foreground();
	void gdr_background();
	void gdr_openProject(const char* project);
	bool gdr_isRunning();
	void gdr_keyDown(int keyCode,int modifiers);
	void gdr_keyUp(int keyCode,int modifiers);
	void gdr_keyChar(const char *keyChar);
	void gdr_mouseDown(int x, int y, int button, int mod);
	void gdr_mouseMove(int x, int y, int mod);
	void gdr_mouseHover(int x, int y, int mod);
	void gdr_mouseUp(int x, int y, int button, int mod);
	void gdr_mouseWheel(int x, int y, int delta, int mod);
	void gdr_touchBegin(int x, int y, int id, int mod);
	void gdr_touchMove(int x, int y, int id, int mod);
	void gdr_touchEnd(int x, int y, int id, int mod);
	void gdr_touchCancel(int x, int y, int id, int mod);
	void gdr_resize(int width, int height, int orientation);
	void gdr_scaleChanged(float s);
	void gdr_dispatchUi(std::function<void()> func, bool wait);
	Windows::UI::Xaml::Controls::SwapChainPanel^ gdr_getRootView();
#ifdef __cplusplus
}
#endif

#endif

