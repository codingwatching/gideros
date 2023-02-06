#include "tttextfield.h"
#include "ttfont.h"
#include "application.h"
#include "texturemanager.h"
#include "ogl.h"
#include <utf8.h>

TTTextField::TTTextField(Application* application, TTFont* font, const char* text, const char* sample, FontBase::TextLayoutParameters *layout) : TextFieldBase(application)
{
	font_ = font;
	if (font_)
		font_->ref();

	data_ = NULL;

	if (text)
		text_ = text;

	if (sample)
		sample_ = sample;

    r_=g_=b_=0;
    a_=1;

    float scalex = application_->getLogicalScaleX();
    float scaley = application_->getLogicalScaleY();

    if (layout)
		layout_=*layout;

    FontBase::TextLayoutParameters empty;
    FontBase::TextLayout l;
    bool isRGB;
    font_->renderFont(sample_.c_str(), &empty, &sminx, &sminy, &smaxx, &smaxy, 0, isRGB, l);
    sminx = sminx/scalex;
    sminy = sminy/scaley;
    smaxx = smaxx/scalex;
    smaxy = smaxy/scaley;


    createGraphics();
}

void TTTextField::cloneFrom(TTTextField *s)
{
    TextFieldBase::cloneFrom(s);
    font_ = s->font_;
    if (font_ != 0)
        font_->ref();
    a_=s->a_;
    r_=s->r_;
    g_=s->g_;
    b_=s->b_;
    minx_=s->minx_;
    miny_=s->miny_;
    maxx_=s->maxx_;
    maxy_=s->maxy_;
    sminx=s->sminx;
    sminy=s->sminy;
    smaxx=s->smaxx;
    smaxy=s->smaxy;

    styleFlags_=s->styleFlags_;

    //data_=s->data_;
    //graphicsBase_=s->graphicsBase_;

    createGraphics();
}

TTTextField::~TTTextField()
{
	if (data_)
	{
		application_->getTextureManager()->destroyTexture(data_);
		data_ = NULL;
	}

	font_->unref();
}

#define FDIF_EPSILON 0.01 //This assumes that logical space coordinates is similar to pixel space, which may not be true at all
#define FDIF(a,b) (((a>b)?(a-b):(b-a))>FDIF_EPSILON)
void TTTextField::createGraphics()
{
	invalidate(INV_GRAPHICS|INV_BOUNDS);
	scaleChanged(); //Mark current scale as graphics scale
    RENDER_LOCK();
    if (data_)
	{
		application_->getTextureManager()->destroyTexture(data_);
		data_ = NULL;
	}

	float lmw=textlayout_.mw;
	float lbh=textlayout_.bh;
	float lw=textlayout_.w;
	float lh=textlayout_.h;
	if (text_.empty())
	{
		graphicsBase_.clear();
		graphicsBase_.getBounds(&minx_, &miny_, &maxx_, &maxy_);
        font_->layoutText("", &layout_,textlayout_);
	}
	else {
		float scalex = application_->getLogicalScaleX();
		float scaley = application_->getLogicalScaleY();

		TextureParameters parameters;
		float smoothing=font_->getSmoothing();
		if (smoothing!=0)
		{
			parameters.filter = eLinear;
			scalex/=smoothing;
			scaley/=smoothing;
		}

		int minx, miny, maxx, maxy;
		bool isRGB;
        unsigned long col=((unsigned long)(r_*0xFF0000)&0xFF0000)|((unsigned long)(g_*0xFF00)&0xFF00)|((unsigned long)(b_*0xFF)&0xFF)|((unsigned long)(a_*0xFF000000)&0xFF000000);
        Dib dib = font_->renderFont(text_.c_str(), &layout_, &minx, &miny, &maxx, &maxy,col,isRGB,textlayout_);
		parameters.format=isRGB?eRGBA8888:eA8;

		if (!sample_.empty())
		{
			maxx = maxx - minx;
			minx = 0;
			miny = miny - sminy*scaley;
			maxy = maxy - sminy*scaley;
		}

		int dx = minx - 1;
		int dy = miny - 1;

		data_ = application_->getTextureManager()->createTextureFromDib(dib, parameters);

		graphicsBase_.data = data_;

		graphicsBase_.mode = ShaderProgram::TriangleStrip;

		graphicsBase_.vertices.resize(4);
		graphicsBase_.vertices[0] = Point2f(dx / scalex,					dy / scaley);
		graphicsBase_.vertices[1] = Point2f((data_->width + dx) / scalex,	dy / scaley);
		graphicsBase_.vertices[2] = Point2f((data_->width + dx) / scalex,	(data_->height + dy) / scaley);
		graphicsBase_.vertices[3] = Point2f(dx / scalex,					(data_->height + dy) / scaley);
		graphicsBase_.vertices.Update();

		float u = (float)data_->width / (float)data_->exwidth;
		float v = (float)data_->height / (float)data_->exheight;

		graphicsBase_.texcoords.resize(4);
		graphicsBase_.texcoords[0] = Point2f(0, 0);
		graphicsBase_.texcoords[1] = Point2f(u, 0);
		graphicsBase_.texcoords[2] = Point2f(u, v);
		graphicsBase_.texcoords[3] = Point2f(0, v);
		graphicsBase_.texcoords.Update();

		graphicsBase_.indices.resize(4);
		graphicsBase_.indices[0] = 0;
		graphicsBase_.indices[1] = 1;
		graphicsBase_.indices[2] = 3;
		graphicsBase_.indices[3] = 2;
		graphicsBase_.indices.Update();

		if (!isRGB)
		{
            graphicsBase_.setColor(r_,g_,b_,a_);
		}

		minx_ = minx/scalex;
		miny_ = miny/scaley;
		maxx_ = maxx/scalex;
		maxy_ = maxy/scaley;
		styleFlags_=textlayout_.styleFlags;
	}
    RENDER_UNLOCK();
    bool layoutSizeChanged=FDIF(textlayout_.mw,lmw)||FDIF(textlayout_.bh,lbh)||FDIF(textlayout_.h,lh)||FDIF(textlayout_.w,lw);
	if (layoutSizeChanged) layoutSizesChanged();
}

void TTTextField::extraBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
	if (minx)
		*minx = minx_;
	if (miny)
		*miny = miny_;
	if (maxx)
		*maxx = maxx_;
	if (maxy)
		*maxy = maxy_;
}

void TTTextField::doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey)
{
    G_UNUSED(sx);
    G_UNUSED(sy);
    G_UNUSED(ex);
    G_UNUSED(ey);
    if (scaleChanged()) createGraphics();
	graphicsBase_.draw(getShader(graphicsBase_.getShaderType()));
}

void TTTextField::setFont(FontBase* font)
{
    if (font->getType() != FontBase::eTTFont) return;

    font_->unref();
    font_ = static_cast<TTFont*>(font);;
    font_->ref();

    prefWidth_=prefHeight_=-1;
    createGraphics();
}

void TTTextField::setText(const char* text)
{
	if (strcmp(text, text_.c_str()) == 0)
		return;

	text_ = text;

    prefWidth_=prefHeight_=-1;
    createGraphics();
}

const char* TTTextField::text() const
{
	return text_.c_str();
}

void TTTextField::setTextColor(float r,float g,float b,float a)
{
    a_ = a;
    r_ = r;
    g_ = g;
    b_ = b;

    if (styleFlags_&TEXTSTYLEFLAG_COLOR) {
        createGraphics();
        graphicsBase_.setColor(1,1,1,1);
    }
    else
        graphicsBase_.setColor(r,g,b,a);
    invalidate(INV_GRAPHICS);
}

void TTTextField::textColor(float &r,float &g,float &b,float &a)
{
    r=r_; g=g_; b=b_; a=a_;
}

void TTTextField::setLetterSpacing(float letterSpacing)
{
	if (layout_.letterSpacing == letterSpacing)
		return;

	layout_.letterSpacing = letterSpacing;
    prefWidth_=prefHeight_=-1;

	createGraphics();
}

float TTTextField::letterSpacing() const
{
	return layout_.letterSpacing;
}

float TTTextField::lineHeight() const
{
    float scaley = application_->getLogicalScaleY();
    return sample_.empty()? 0 : smaxy - sminy;
}

void TTTextField::setSample(const char* sample)
{
    sample_ = sample;

    float scalex = application_->getLogicalScaleX();
    float scaley = application_->getLogicalScaleY();
	float smoothing=font_->getSmoothing();
    if (smoothing!=0)
    {
        scalex/=smoothing;
        scaley/=smoothing;
    }

    FontBase::TextLayoutParameters empty;
    FontBase::TextLayout l;
    bool isRGB;
    font_->renderFont(sample, &empty, &sminx, &sminy, &smaxx, &smaxy,0,isRGB,l);
    sminx = sminx/scalex;
    sminy = sminy/scaley;
    smaxx = smaxx/scalex;
    smaxy = smaxy/scaley;

    createGraphics();
}

const char* TTTextField::sample() const
{
    return sample_.c_str();
}

void TTTextField::setLayout(FontBase::TextLayoutParameters *l)
{
	if (l)
	{
        prefWidth_=prefHeight_=-1;
        layout_=*l;
		createGraphics();
	}
}
