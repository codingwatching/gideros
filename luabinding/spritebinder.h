#ifndef SPRITEBINDER_H
#define SPRITEBINDER_H

#include "binder.h"

class GhostSprite;
class SpriteBinder
{
public:
	SpriteBinder(lua_State* L);

private:
	static int create(lua_State* L);
	static int destruct(void *p);
    static int clone(lua_State *L);

	static int addChild(lua_State* L);
	static int addChildAt(lua_State* L);
	static int removeChild(lua_State* L);
	static int removeChildAt(lua_State* L);
	static int numChildren(lua_State* L);
    static int swapChildren(lua_State* L);
	static int swapChildrenAt(lua_State* L);
	static int getChildAt(lua_State* L);
	static int getChildrenAtPoint(lua_State* L);
	static int getParent(lua_State* L);
	static int contains(lua_State* L);
	static int getChildIndex(lua_State* L);
	static int removeFromParent(lua_State* L);
	
	static int setClip(lua_State* L);
    static int getClip(lua_State* L);
    static int setLayoutParameters(lua_State *L);
    static int setLayoutConstraints(lua_State *L);
    static int getLayoutParameters(lua_State *L);
    static int getLayoutConstraints(lua_State *L);
    static int getLayoutInfo(lua_State *L);

	static int getX(lua_State* L);
	static int getY(lua_State* L);
	static int getZ(lua_State* L);
	static int getRotation(lua_State* L);
	static int getRotationX(lua_State* L);
	static int getRotationY(lua_State* L);
	static int getScaleX(lua_State* L);
	static int getScaleY(lua_State* L);
	static int getScaleZ(lua_State* L);
    static int getSkewX(lua_State* L);
    static int getSkewY(lua_State* L);

	static int setX(lua_State* L);
	static int setY(lua_State* L);
	static int setZ(lua_State* L);
	static int setRotation(lua_State* L);
	static int setRotationX(lua_State* L);
	static int setRotationY(lua_State* L);
	static int setScaleX(lua_State* L);
	static int setScaleY(lua_State* L);
	static int setScaleZ(lua_State* L);
    static int setSkewX(lua_State* L);
    static int setSkewY(lua_State* L);
    static int lookAt(lua_State* L);

	static int setPosition(lua_State* L);
	static int getPosition(lua_State* L);

    static int setAnchorPosition(lua_State* L);
    static int getAnchorPosition(lua_State* L);
	
    static int setAnchorPoint(lua_State* L);
    static int getAnchorPoint(lua_State* L);

	static int setScale(lua_State* L);
	static int getScale(lua_State* L);

    static int setSkew(lua_State* L);
    static int getSkew(lua_State* L);

	static int set(lua_State* L);
	static int get(lua_State* L);

	static int localToGlobal(lua_State* L);
    static int globalToLocal(lua_State* L);
    static int spriteToLocal(lua_State* L);
    static int spriteToLocalMatrix(lua_State* L);

	static int isVisible(lua_State* L);
    static int setVisible(lua_State* L);
    static int setAutoSort(lua_State* L);

	static int isOnStage(lua_State* L);

	static int getColorTransform(lua_State* L);
	static int setColorTransform(lua_State* L);

	static int hitTestPoint(lua_State* L);

	static int getWidth(lua_State* L);
	static int getHeight(lua_State* L);
	static int getSize(lua_State* L);

	static int getMatrix(lua_State* L);
	static int setMatrix(lua_State* L);

	static int getAlpha(lua_State* L);
	static int setAlpha(lua_State* L);

	static int getBounds(lua_State* L);

    static int setBlendMode(lua_State* L);
    static int clearBlendMode(lua_State* L);

	static int setShader(lua_State* L);
	static int setShaderConstant(lua_State* L);
	static int setStencilOperation(lua_State* L);
	static int setEffectStack(lua_State* L);
	static int setEffectConstant(lua_State* L);
	static int redrawEffects(lua_State* L);

	static int setStopEventPropagation(lua_State *L);
    static int getDrawCount(lua_State* L);
    static int setHiddenChildren(lua_State* L);
    static int setCheckClip(lua_State* L);
    static int setWorldAlign(lua_State* L);

    static int setStyle(lua_State* L);
    static int resolveStyle(lua_State* L);

    static int setGhosts(lua_State* L);
    static int __parseGhosts(lua_State* L);

    static int layoutStrings[64];    
public:
    static void __parseGhost(GhostSprite *g,lua_State* L);
    static size_t tokenChildren;
    static size_t tokenParent;
    static size_t tokenNewclone;
    static size_t tokenUserdata;
    static size_t tokenInsets;
    static size_t tokenFill;
    static size_t tokenWidth;
    static size_t tokenHeight;
    static int updateStyle(lua_State* L);
};

#endif

