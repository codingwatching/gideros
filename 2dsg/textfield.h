#ifndef TEXTFIELD_H
#define TEXTFIELD_H

#include "font.h"
#include "sprite.h"
#include "graphicsbase.h"
#include "textfieldbase.h"

#include <string>

class Application;

class TextField : public TextFieldBase
{
public:
    TextField(Application *application, BMFontBase* font=NULL, const char* text=NULL, const char *sample=NULL, FontBase::TextLayoutParameters *params=NULL);
    virtual Sprite *clone() { TextField *clone=new TextField(application_); clone->cloneFrom(this); return clone; }
    void cloneFrom(TextField *);
    void bulkUpdate(bool en);

	virtual ~TextField()
	{
		if (font_ != 0)
			font_->unref();
	}

    virtual void setFont(FontBase* font);
    FontBase *getFont() { return font_; };

	virtual void setText(const char* text);
	virtual const char* text() const;

    virtual void setTextColor(float r,float g,float b,float a);
    virtual void textColor(float &r,float &g,float &b,float &a);

    virtual void setLetterSpacing(float letterSpacing);
    virtual float letterSpacing() const;

    virtual float lineHeight() const;

    virtual void setSample(const char* sample);
    virtual const char* sample() const;

    virtual void setLayout(FontBase::TextLayoutParameters *l=NULL);

	void createGraphics();

private:
	virtual void extraBounds(float* minx, float* miny, float* maxx, float* maxy) const;

    BMFontBase* font_;

	float a_, r_, g_, b_;
	bool bulk;
	bool dirty;

private:
	std::vector<GraphicsBase> graphicsBase_;
    virtual void doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey);
    float minx_, miny_, maxx_, maxy_;
    int sminx, sminy, smaxx, smaxy;
};

#endif
