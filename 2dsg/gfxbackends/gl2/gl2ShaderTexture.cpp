/*
 * gl2ShaderTexture.cpp
 *
 *  Created on: 5 juin 2015
 *      Author: Nicolas
 */

#include "gl2Shaders.h"

ogl2ShaderTexture::ogl2ShaderTexture(ShaderTexture::Format format,ShaderTexture::Packing packing,int width,int height,const void *data,ShaderTexture::Wrap wrap,ShaderTexture::Filtering filtering)
{
	GLCALL_INIT;
	glid=0;
	native=false;
	this->width=width;
	this->height=height;

    GLint oldTex = 0;
    GLCALL glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);

    GLCALL glGenTextures(1, &glid);
    GLCALL glBindTexture(GL_TEXTURE_2D, glid);
    switch (wrap)
    {
    case WRAP_CLAMP:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case WRAP_REPEAT:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    }
    switch (filtering)
    {
    case FILT_NEAREST:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case FILT_LINEAR:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case FILT_LINEAR_MIPMAP:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }

    GLuint glformat=GL_RGBA;
    switch (format)
    {
    	case FMT_ALPHA: glformat=GL_ALPHA; break;
    	case FMT_RGB: glformat=GL_RGB; break;
    	case FMT_RGBA: glformat=GL_RGBA; break;
    	case FMT_Y: glformat=GL_LUMINANCE; break;
    	case FMT_YA: glformat=GL_LUMINANCE_ALPHA; break;
    	case FMT_DEPTH: glformat=GL_DEPTH_COMPONENT; break;
    	case FMT_NATIVE: break; // Format is defined elsewhere, there should be no data yet
    }
    GLuint gltype=GL_UNSIGNED_BYTE;
    switch (packing)
    {
    	case PK_UBYTE: gltype=GL_UNSIGNED_BYTE; break;
    	case PK_USHORT_565: gltype=GL_UNSIGNED_SHORT_5_6_5; break;
    	case PK_USHORT_4444: gltype=GL_UNSIGNED_SHORT_4_4_4_4; break;
    	case PK_USHORT_5551: gltype=GL_UNSIGNED_SHORT_5_5_5_1; break;
    	case PK_USHORT: gltype=GL_UNSIGNED_SHORT; break;
    	case PK_UINT: gltype=GL_UNSIGNED_INT; break;
    	case PK_FLOAT: gltype=GL_FLOAT; break;
    }
    GLuint iformat=glformat;
    const void *idata=data;
    if (ogl2ShaderEngine::isGLES&&(ogl2ShaderEngine::version>=3)) {
    	if (glformat==GL_DEPTH_COMPONENT) {
    		iformat=GL_DEPTH_COMPONENT32F;
    		idata=NULL; //Don't supply data for depth component
    	}
    }
    if (data) {
    	GLCALL glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, glformat, gltype, idata);
        if (filtering==FILT_LINEAR_MIPMAP)
            GLCALL glGenerateMipmap(GL_TEXTURE_2D);
    }

    if (((!ogl2ShaderEngine::isGLES)||(ogl2ShaderEngine::version>=3))&&(format==FMT_DEPTH)) {
    	if (filtering==FILT_LINEAR) {
        	//Filtering enabled with depth: assume shadow sampling
    		GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE , GL_COMPARE_REF_TO_TEXTURE);
    		GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC , GL_LEQUAL);
    	}
        else {
    		GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE , GL_NONE);
    	}
    }

    GLCALL glBindTexture(GL_TEXTURE_2D, oldTex);
}

void ogl2ShaderTexture::updateData(ShaderTexture::Format format,ShaderTexture::Packing packing,int width,int height,const void *data,ShaderTexture::Wrap wrap,ShaderTexture::Filtering filtering)
{
	GLCALL_INIT;
	native=false;
	this->width=width;
	this->height=height;

    GLint oldTex = 0;
    GLCALL glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);

    GLCALL glBindTexture(GL_TEXTURE_2D, glid);
    switch (wrap)
    {
    case WRAP_CLAMP:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case WRAP_REPEAT:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    }
    switch (filtering)
    {
    case FILT_NEAREST:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case FILT_LINEAR:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case FILT_LINEAR_MIPMAP:
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLCALL glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }

    GLuint glformat=GL_RGBA;
    switch (format)
    {
    	case FMT_ALPHA: glformat=GL_ALPHA; break;
    	case FMT_RGB: glformat=GL_RGB; break;
    	case FMT_RGBA: glformat=GL_RGBA; break;
    	case FMT_Y: glformat=GL_LUMINANCE; break;
    	case FMT_YA: glformat=GL_LUMINANCE_ALPHA; break;
    	case FMT_DEPTH: glformat=GL_DEPTH_COMPONENT; break;
    	case FMT_NATIVE: break; // Format is defined elsewhere, there should be no data
    }
    GLuint gltype=GL_UNSIGNED_BYTE;
    switch (packing)
    {
    	case PK_UBYTE: gltype=GL_UNSIGNED_BYTE; break;
    	case PK_USHORT_565: gltype=GL_UNSIGNED_SHORT_5_6_5; break;
    	case PK_USHORT_4444: gltype=GL_UNSIGNED_SHORT_4_4_4_4; break;
    	case PK_USHORT_5551: gltype=GL_UNSIGNED_SHORT_5_5_5_1; break;
    	case PK_USHORT: gltype=GL_UNSIGNED_SHORT; break;
    	case PK_UINT: gltype=GL_UNSIGNED_INT; break;
    	case PK_FLOAT: gltype=GL_FLOAT; break;
    }
    GLuint iformat=glformat;
    const void *idata=data;
    if (ogl2ShaderEngine::isGLES&&(ogl2ShaderEngine::version>=3)) {
    	if (glformat==GL_DEPTH_COMPONENT) {
    		iformat=GL_DEPTH_COMPONENT24;
    		idata=NULL; //Don't supply data for depth component
    	}
    }
    if (data) {
    	GLCALL glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, glformat, gltype, idata);
        if (filtering==FILT_LINEAR_MIPMAP)
            GLCALL glGenerateMipmap(GL_TEXTURE_2D);
    }

    GLCALL glBindTexture(GL_TEXTURE_2D, oldTex);
}

void ogl2ShaderTexture::setNative(void *externalTexture)
{
	GLCALL_INIT;
	if (!native)
		GLCALL glDeleteTextures(1,&glid);
	glid=externalTexture?(*((GLuint *)externalTexture)):0;
	native=true;
}

void *ogl2ShaderTexture::getNative()
{
	return &glid;
}

ogl2ShaderTexture::~ogl2ShaderTexture()
{
    GLCALL_CHECK;
    GLCALL_INIT;
	if (!native)
		GLCALL glDeleteTextures(1,&glid);
}



