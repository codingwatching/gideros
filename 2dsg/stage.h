#ifndef STAGE_H
#define STAGE_H

#include "sprite.h"
#include "touch.h"
#include <set>
#include "touchevent.h"

class Application;

class Stage : public Sprite
{
public:
    Stage(Application* application) : Sprite(application)
	{
		startTime_ = -1;
        isSpritesWithListenersDirty_ = true;
        needLayout=false;
	}
    bool needLayout;

	void enterFrame(int deltaFrameCount = -1, double lastFrameRenderTime=0);

    void mouseDown(int x, int y, int button, int modifiers, int type, float sx, float sy, float tx, float ty);
    void mouseUp(int x, int y, int button, int modifiers, int type, float sx, float sy, float tx, float ty);
    void mouseMove(int x, int y, int button, int modifiers, int type, float sx, float sy, float tx, float ty);
    void mouseHover(int x, int y, int button, int modifiers, int type, float sx, float sy, float tx, float ty);
    void mouseWheel(int x, int y, int modifiers, float sx, float sy, float tx, float ty, int wheel);
    void mouseEnter(int x, int y, int buttons, int modifiers, float sx, float sy, float tx, float ty);
    void mouseLeave(int x, int y, int modifiers, float sx, float sy, float tx, float ty);

    void touchesBegin(ginput_TouchEvent *event, float sx, float sy, float tx, float ty);
    void touchesMove(ginput_TouchEvent *event, float sx, float sy, float tx, float ty);
    void touchesEnd(ginput_TouchEvent *event, float sx, float sy, float tx, float ty);
    void touchesCancel(ginput_TouchEvent *event, float sx, float sy, float tx, float ty);

    void keyDown(int keyCode, int realCode, int modifiers);
    void keyUp(int keyCode, int realCode, int modifiers);
    void keyChar(const char *code);

    void validateLayout();
    void validateEffects();


    Application* application() const
    {
        return application_;
    }

	virtual bool isStage() const
	{
		return true;
	}

    void setSpritesWithListenersDirty()
    {
        isSpritesWithListenersDirty_ = true;
    }

private:
	double startTime_;
	double lastTime_;
	int lastFrameCount_;

private:
    void populateSpritesWithListeners();
    std::vector<Sprite*> spritesWithListeners_;
    bool isSpritesWithListenersDirty_;
    void dispatchToSpritesWithListeners(Event *event,int mask);
    void getDimensions(float& w,float &h);
};

#endif
