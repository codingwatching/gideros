#ifndef TEXTURE_PACKER_H
#define TEXTURE_PACKER_H

class TexturePacker
{
public:
	virtual ~TexturePacker() {}
	virtual void setTextureCount(int tcount) = 0;
	virtual bool addTexture(int width, int height) = 0;
	virtual void packTextures(int* width, int* height, int border, bool forceSquare = false) = 0;
    virtual int getTextureLocation(int index, int* x, int* y, int* width, int* height) = 0;
};

TexturePacker* createTexturePacker(void);
TexturePacker* createProgressiveTexturePacker(int width,int height);
void releaseTexturePacker(TexturePacker* tp);

#endif
