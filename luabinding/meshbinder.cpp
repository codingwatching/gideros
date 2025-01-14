#include "meshbinder.h"
#include <gmesh.h>
#include "luaapplication.h"
#include <luautil.h>

MeshBinder::MeshBinder(lua_State *L)
{
    Binder binder(L);

    static const luaL_Reg functionList[] = {
        {"setVertex", setVertex},
        {"setIndex", setIndex},
        {"setColor", setColor},
        {"setTextureCoordinate", setTextureCoordinate},

        {"setVertices", setVertices},
        {"setIndices", setIndices},
        {"setColors", setColors},
        {"setTextureCoordinates", setTextureCoordinates},

        {"setVertexArray", setVertexArray},
        {"setIndexArray", setIndexArray},
        {"setColorArray", setColorArray},
        {"setTextureCoordinateArray", setTextureCoordinateArray},
        {"setGenericArray", setGenericArray},

        {"resizeVertexArray", resizeVertexArray},
        {"resizeIndexArray", resizeIndexArray},
        {"resizeColorArray", resizeColorArray},
        {"resizeTextureCoordinateArray", resizeTextureCoordinateArray},

        {"clearVertexArray", clearVertexArray},
        {"clearIndexArray", clearIndexArray},
        {"clearColorArray", clearColorArray},
        {"clearTextureCoordinateArray", clearTextureCoordinateArray},

        {"getVertexArraySize", getVertexArraySize},
        {"getIndexArraySize", getIndexArraySize},
        {"getColorArraySize", getColorArraySize},
        {"getTextureCoordinateArraySize", getTextureCoordinateArraySize},

        {"getVertex", getVertex},
        {"getIndex", getIndex},
        {"getColor", getColor},
        {"getTextureCoordinate", getTextureCoordinate},

        {"setTexture", setTexture},
        {"clearTexture", clearTexture},
        {"setPrimitiveType", setPrimitiveType},
	    {"setInstanceCount",setInstanceCount},
	    {"setCullMode",setCullMode},

        {NULL, NULL},
    };

    binder.createClass("Mesh", "Sprite", create, destruct, functionList);

    lua_getglobal(L, "Mesh");

    lua_pushinteger(L, ShaderProgram::Point);
    lua_setfield(L, -2, "PRIMITIVE_POINT");
    lua_pushinteger(L, ShaderProgram::Lines);
    lua_setfield(L, -2, "PRIMITIVE_LINES");
    lua_pushinteger(L, ShaderProgram::LineLoop);
    lua_setfield(L, -2, "PRIMITIVE_LINELOOP");
    lua_pushinteger(L, ShaderProgram::Triangles);
    lua_setfield(L, -2, "PRIMITIVE_TRIANGLES");
    lua_pushinteger(L, ShaderProgram::TriangleStrip);
    lua_setfield(L, -2, "PRIMITIVE_TRIANGLESTRIP");

    lua_pop(L, 1);
}

static int decodeType(lua_State *L,int idx) {
	const char *tstr=luaL_checkstring(L,idx);
	int t=0;
	while (*tstr) {
		switch (*tstr) {
		case '>': t|=0x80; break; //BE
		case '<': break; //LE, default
		case 'f':
		case 'F':
			t|=0x14; return t; //Float
		case 'd':
		case 'D':
			t|=0x18; return t; //Double
		case 'b':
			t|=0x01; return t; //Byte
		case 'B':
			t|=0x21; return t; //UByte
		case 's':
			t|=0x02; return t; //Short
		case 'S':
			t|=0x22; return t; //UShort
		case 'i':
			t|=0x04; return t; //Int/Long
		case 'I':
			t|=0x24; return t; //UInt/ULong
		}
		tstr++;
	}
	return 0;
}

static void toFloatComponents(lua_State *L,int idx,int mult,std::vector<float> &v) {
	int i=1;
	int isBE=(!*((char *)&i));
	size_t sl;
	const char *s=luaL_checklstring(L,idx,&sl);
	int type=decodeType(L,idx+1);
	int big=type&0x80;
	int bswap=(isBE&&(!big))||(big&&(!isBE));
	int vlen=(type&0x0F);
	int vt=type&0x3F;
	if (!type) {
		lua_pushstring(L,"Bad component type");
		lua_error(L);
	}
	sl=sl/mult;
	sl=sl/(type&0x0F);
	sl*=mult;
	v.resize(sl);
	int vi=0;
	while (sl) {
		 char vbytes[8];
		 if (bswap) {
			 for (i=0;i<vlen;i++)
				 vbytes[i]=s[vlen-1-i];
		 }
		 else
			 memcpy(vbytes,s,vlen);
		 s+=vlen;
		 sl--;

	#define FTYPE(c,t) \
			 case c: \
			 { \
				 t m; \
				 memcpy(&m,vbytes,vlen); \
				 v[vi++]=m; \
				 break; \
			 }
		 switch (vt) {
			 FTYPE(0x01,int8_t);
			 FTYPE(0x21,uint8_t);
			 FTYPE(0x02,int16_t);
			 FTYPE(0x22,uint16_t);
			 FTYPE(0x04,int32_t);
			 FTYPE(0x24,uint32_t);
			 FTYPE(0x14,float);
			 FTYPE(0x18,double);
		 }
	#undef FTYPE
	}
}

static void toIntComponents(lua_State *L,int idx,int mult,std::vector<unsigned int> &v) {
	int i=1;
	int isBE=(!*((char *)&i));
	size_t sl;
	const char *s=luaL_checklstring(L,idx,&sl);
	int type=decodeType(L,idx+1);
	int big=type&0x80;
	int bswap=(isBE&&(!big))||(big&&(!isBE));
	int vlen=(type&0x0F);
	int vt=type&0x3F;
	if (!type) {
		lua_pushstring(L,"Bad component type");
		lua_error(L);
	}
	sl=sl/mult;
	sl=sl/(type&0x0F);
	sl*=mult;
	v.resize(sl);
	int vi=0;
	while (sl) {
		 char vbytes[8];
		 if (bswap) {
			 for (i=0;i<vlen;i++)
				 vbytes[i]=s[vlen-1-i];
		 }
		 else
			 memcpy(vbytes,s,vlen);
		 s+=vlen;
		 sl--;

	#define FTYPE(c,t) \
			 case c: \
			 { \
				 t m; \
				 memcpy(&m,vbytes,vlen); \
				 v[vi++]=m; \
				 break; \
			 }
		 switch (vt) {
			 FTYPE(0x01,int8_t);
			 FTYPE(0x21,uint8_t);
			 FTYPE(0x02,int16_t);
			 FTYPE(0x22,uint16_t);
			 FTYPE(0x04,int32_t);
			 FTYPE(0x24,uint32_t);
			 FTYPE(0x14,float);
			 FTYPE(0x18,double);
		 }
	#undef FTYPE
	}
}

int MeshBinder::create(lua_State *L)
{
    LuaApplication* application = static_cast<LuaApplication*>(luaL_getdata(L));

    bool is3d = lua_toboolean(L, 1);

    Binder binder(L);

    binder.pushInstance("Mesh", new GMesh(application->getApplication(),is3d));

    return 1;
}

int MeshBinder::destruct(void *p)
{
    void* ptr = GIDEROS_DTOR_UDATA(p);
    GMesh* mesh = static_cast<GMesh*>(ptr);
    mesh->unref();

    return 0;
}

int MeshBinder::setVertex(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    int i = luaL_checkinteger(L, 2) - 1;
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    float z = luaL_optnumber(L, 5, 0.0);

    mesh->setVertex(i, x, y, z);

    return 0;
}

int MeshBinder::setIndex(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    int i = luaL_checkinteger(L, 2) - 1;
    int index = luaL_checkinteger(L, 3) - 1;

    mesh->setIndex(i, index);

    return 0;
}

int MeshBinder::setColor(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    int i = luaL_checkinteger(L, 2) - 1;
    unsigned int color = luaL_checkinteger(L, 3);
    float alpha = luaL_optnumber(L, 4, 1.0);

    mesh->setColor(i, color, alpha);

    return 0;
}

int MeshBinder::setTextureCoordinate(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    int i = luaL_checkinteger(L, 2) - 1;
    float u = luaL_checknumber(L, 3);
    float v = luaL_checknumber(L, 4);

    mesh->setTextureCoordinate(i, u, v);

    return 0;
}

int MeshBinder::setVertices(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    
    bool is3d=mesh->is3d();
    int order=is3d?4:3;

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        for (int k = 0; k < n/order; ++k)
        {
            lua_rawgeti(L, 2, k * order + 1);
            int i = luaL_checkinteger(L, -1) - 1;
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * order + 2);
            float x = luaL_checknumber(L, -1);
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * order + 3);
            float y = luaL_checknumber(L, -1);
            lua_pop(L, 1);

            float z=0;
            if (is3d)
            {
            	lua_rawgeti(L, 2, k * order + 4);
            	z = luaL_checknumber(L, -1);
            	lua_pop(L, 1);
            }
            mesh->setVertex(i, x, y, z);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        for (int k = 0; k < n/order; ++k)
        {
            int i = luaL_checkinteger(L, k * order + 2) - 1;
            float x = luaL_checknumber(L, k * order + 3);
            float y = luaL_checknumber(L, k * order + 4);
            float z=0;
            if (is3d) 
            	z= luaL_checknumber(L, k * order + 5);
            mesh->setVertex(i, x, y, z);
        }
    }

    return 0;
}

int MeshBinder::setIndices(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        for (int k = 0; k < n/2; ++k)
        {
            lua_rawgeti(L, 2, k * 2 + 1);
            int i = luaL_checkinteger(L, -1) - 1;
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * 2 + 2);
            int index = luaL_checknumber(L, -1) - 1;
            lua_pop(L, 1);

            mesh->setIndex(i, index);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        for (int k = 0; k < n/2; ++k)
        {
            int i = luaL_checkinteger(L, k * 2 + 2) - 1;
            int index = luaL_checknumber(L, k * 2 + 3) - 1;

            mesh->setIndex(i, index);
        }
    }

    return 0;
}

int MeshBinder::setColors(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        for (int k = 0; k < n/3; ++k)
        {
            lua_rawgeti(L, 2, k * 3 + 1);
            int i = luaL_checkinteger(L, -1) - 1;
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * 3 + 2);
            unsigned int color = luaL_checkinteger(L, -1);
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * 3 + 3);
            float alpha = luaL_checknumber(L, -1);
            lua_pop(L, 1);

            mesh->setColor(i, color, alpha);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        for (int k = 0; k < n/3; ++k)
        {
            int i = luaL_checkinteger(L, k * 3 + 2) - 1;
            unsigned int color = luaL_checkinteger(L, k * 3 + 3);
            float alpha = luaL_checknumber(L, k * 3 + 4);

            mesh->setColor(i, color, alpha);
        }
    }

    return 0;
}

int MeshBinder::setTextureCoordinates(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        for (int k = 0; k < n/3; ++k)
        {
            lua_rawgeti(L, 2, k * 3 + 1);
            int i = luaL_checkinteger(L, -1) - 1;
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * 3 + 2);
            float u = luaL_checknumber(L, -1);
            lua_pop(L, 1);

            lua_rawgeti(L, 2, k * 3 + 3);
            float v = luaL_checknumber(L, -1);
            lua_pop(L, 1);

            mesh->setTextureCoordinate(i, u, v);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        for (int k = 0; k < n/3; ++k)
        {
            int i = luaL_checkinteger(L, k * 3 + 2) - 1;
            float u = luaL_checknumber(L, k * 3 + 3);
            float v = luaL_checknumber(L, k * 3 + 4);

            mesh->setTextureCoordinate(i, u, v);
        }
    }

    return 0;
}

int MeshBinder::setGenericArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int index=luaL_checkinteger(L,2);
    ShaderProgram::DataType type=(ShaderProgram::DataType) luaL_checkinteger(L,3);
    int mult=luaL_checkinteger(L,4);
    int count=luaL_checkinteger(L,5);
    size_t blen;
    void *bptr=lua_tobuffer(L,6,&blen);
    int offset=luaL_optinteger(L,7,0);
    int stride=luaL_optinteger(L,8,0);
    int master=luaL_optinteger(L,9,-1);
    if (bptr!=NULL)
    {
        int elesz=1;
        switch (type)
        {
        case ShaderProgram::DBYTE:
        case ShaderProgram::DUBYTE:
            break;
        case ShaderProgram::DSHORT:
        case ShaderProgram::DUSHORT:
            elesz=2;
            break;
        case ShaderProgram::DINT:
        case ShaderProgram::DFLOAT:
            elesz=4;
            break;
        }
        if (blen!=(mult*count*elesz))
        {
        	lua_pushstring(L,"Actual buffer length doesn't match size multiple and count values");
        	lua_error(L);
        }
        mesh->setGenericArray(index,bptr,type,mult,count,offset,stride,master);
    }
    else {
		luaL_checktype(L, 6, LUA_TTABLE);

		int n = lua_objlen(L, 6);  /* get size of table */
		if (n!=(mult*count))
		{
			lua_pushstring(L,"Actual array length doesn't match size multiple and count values");
			lua_error(L);
		}

		void *ptr = 0;
		switch (type)
		{
		case ShaderProgram::DBYTE:
		case ShaderProgram::DUBYTE:
		{
			char *p=(char *) malloc(mult*count);
			ptr=p;
			for (int i=1; i<=n; i++) {
				lua_rawgeti(L, 6, i);  /* push t[i] */
				*(p++)=luaL_checkinteger(L,-1);
				lua_pop(L,1);
			  }
			break;
		}
		case ShaderProgram::DSHORT:
		case ShaderProgram::DUSHORT:
		{
			short *p=(short *) malloc(mult*count*2);
			ptr=p;
			for (int i=1; i<=n; i++) {
				lua_rawgeti(L, 6, i);  /* push t[i] */
				*(p++)=luaL_checkinteger(L,-1);
				lua_pop(L,1);
			  }
			break;
		}
		case ShaderProgram::DINT:
		{
			int *p=(int *) malloc(mult*count*4);
			ptr=p;
			for (int i=1; i<=n; i++) {
				lua_rawgeti(L, 6, i);  /* push t[i] */
				*(p++)=luaL_checkinteger(L,-1);
				lua_pop(L,1);
			  }
			break;
		}
		case ShaderProgram::DFLOAT:
		{
			float *p=(float *) malloc(mult*count*4);
			ptr=p;
			for (int i=1; i<=n; i++) {
				lua_rawgeti(L, 6, i);  /* push t[i] */
				*(p++)=luaL_checknumber(L,-1);
				lua_pop(L,1);
			  }
			break;
		}
		}

        mesh->setGenericArray(index,ptr,type,mult,count,offset,stride,master);
		free(ptr);
    }
	return 0;
}

int MeshBinder::setVertexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    std::vector<float> vertices;
    
    int order=mesh->is3d()?3:2;
    if (lua_type(L, 2) == LUA_TSTRING)
    	toFloatComponents(L,2,order,vertices);
	else if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        n = (n / order) * order;
        vertices.resize(n);
        for (int i = 0; i < n; ++i)
        {
            lua_rawgeti(L, 2, i + 1);
            vertices[i] = luaL_checknumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        n = (n / order) * order;
        vertices.resize(n);
        for (int i = 0; i < n; ++i)
            vertices[i] = luaL_checknumber(L, i + 2);
    }

	if (vertices.size() > 0)
		mesh->setVertexArray(&vertices[0], vertices.size());
	else
		mesh->clearVertexArray();

    return 0;
}

int MeshBinder::setIndexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    std::vector<unsigned int> indices;

    if (lua_type(L, 2) == LUA_TSTRING)
    	toIntComponents(L,2,1,indices);
    else if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        indices.resize(n);
        for (int i = 0; i < n; ++i)
        {
            lua_rawgeti(L, 2, i + 1);
            indices[i] = luaL_checkinteger(L, -1) - 1;
            lua_pop(L, 1);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        indices.resize(n);
        for (int i = 0; i < n; ++i)
            indices[i] = luaL_checkinteger(L, i + 2) - 1;
    }

	if (indices.size() > 0)
		mesh->setIndexArray(&indices[0], indices.size());
	else
		mesh->clearIndexArray();

    return 0;
}

int MeshBinder::setColorArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    std::vector<unsigned int> colors;
    std::vector<float> alphas;

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        n /= 2;
        colors.resize(n);
        alphas.resize(n);
        for (int i = 0; i < n; ++i)
        {
            lua_rawgeti(L, 2, i * 2 + 1);
            colors[i] = luaL_checkinteger(L, -1);
            lua_pop(L, 1);

            lua_rawgeti(L, 2, i * 2 + 2);
            alphas[i] = luaL_checknumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        n /= 2;
        colors.resize(n);
        alphas.resize(n);
        for (int i = 0; i < n; ++i)
        {
            colors[i] = luaL_checkinteger(L, i * 2 + 2);
            alphas[i] = luaL_checknumber(L, i * 2 + 3);
        }
    }

	if (colors.size() > 0)
		mesh->setColorArray(&colors[0], &alphas[0], colors.size());
	else
		mesh->clearColorArray();

    return 0;
}

int MeshBinder::setTextureCoordinateArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    std::vector<float> textureCoordinates;

    if (lua_type(L, 2) == LUA_TSTRING)
    	toFloatComponents(L,2,2,textureCoordinates);
    else if (lua_type(L, 2) == LUA_TTABLE)
    {
        int n = lua_objlen(L, 2);
        n = (n / 2) * 2;
        textureCoordinates.resize(n);
        for (int i = 0; i < n; ++i)
        {
            lua_rawgeti(L, 2, i + 1);
            textureCoordinates[i] = luaL_checknumber(L, -1);
            lua_pop(L, 1);
        }
    }
    else
    {
        int n = lua_gettop(L) - 1;
        n = (n / 2) * 2;
        textureCoordinates.resize(n);
        for (int i = 0; i < n; ++i)
            textureCoordinates[i] = luaL_checknumber(L, i + 2);
    }

	if (textureCoordinates.size()>0)
		mesh->setTextureCoordinateArray(&textureCoordinates[0], textureCoordinates.size());
	else
		mesh->clearTextureCoordinateArray();

    return 0;
}


int MeshBinder::resizeVertexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->resizeVertexArray(luaL_checkinteger(L, 2));

    return 0;
}

int MeshBinder::resizeIndexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->resizeIndexArray(luaL_checkinteger(L, 2));

    return 0;
}

int MeshBinder::resizeColorArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->resizeColorArray(luaL_checkinteger(L, 2));

    return 0;
}

int MeshBinder::resizeTextureCoordinateArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->resizeTextureCoordinateArray(luaL_checkinteger(L, 2));

    return 0;
}


int MeshBinder::clearVertexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->clearVertexArray();

    return 0;
}

int MeshBinder::clearIndexArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->clearIndexArray();

    return 0;
}

int MeshBinder::clearColorArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->clearColorArray();

    return 0;
}

int MeshBinder::clearTextureCoordinateArray(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    mesh->clearTextureCoordinateArray();

    return 0;
}

int MeshBinder::getVertexArraySize(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    lua_pushinteger(L, mesh->getVertexArraySize());

    return 1;
}

int MeshBinder::getIndexArraySize(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    lua_pushinteger(L, mesh->getIndexArraySize());

    return 1;
}

int MeshBinder::getColorArraySize(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    lua_pushinteger(L, mesh->getColorArraySize());

    return 1;
}

int MeshBinder::getTextureCoordinateArraySize(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));

    lua_pushinteger(L, mesh->getTextureCoordinateArraySize());

    return 1;
}

int MeshBinder::getVertex(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int i = luaL_checkinteger(L, 2) - 1;

    if (i < 0 || i >= mesh->getVertexArraySize())
        luaL_error(L, "The supplied index is out of bounds.");

    int order=mesh->is3d()?3:2;
    float x, y, z;
    mesh->getVertex(i, &x, &y, &z);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (order==3)
    	lua_pushnumber(L, z);

    return order;
}

int MeshBinder::getIndex(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int i = luaL_checkinteger(L, 2) - 1;

    if (i < 0 || i >= mesh->getIndexArraySize())
        luaL_error(L, "The supplied index is out of bounds.");

    unsigned int index;
    mesh->getIndex(i, &index);
    lua_pushinteger(L, (int)index + 1);

    return 1;
}

int MeshBinder::getColor(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int i = luaL_checkinteger(L, 2) - 1;

    if (i < 0 || i >= mesh->getColorArraySize())
        luaL_error(L, "The supplied index is out of bounds.");

    unsigned int color;
    float alpha;
    mesh->getColor(i, &color, &alpha);
    lua_pushinteger(L, color);
    lua_pushnumber(L, alpha);

    return 2;
}

int MeshBinder::getTextureCoordinate(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int i = luaL_checkinteger(L, 2) - 1;

    if (i < 0 || i >= mesh->getTextureCoordinateArraySize())
        luaL_error(L, "The supplied index is out of bounds.");

    float u, v;
    mesh->getTextureCoordinate(i, &u, &v);
    lua_pushnumber(L, u);
    lua_pushnumber(L, v);

    return 2;
}

int MeshBinder::setPrimitiveType(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    ShaderProgram::ShapeType prim=(ShaderProgram::ShapeType) luaL_checkinteger(L,2);

    mesh->setPrimitiveType(prim);

    return 0;
}

int MeshBinder::setInstanceCount(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    size_t ic=(size_t) luaL_checkinteger(L,2);

    mesh->setInstanceCount(ic);

    return 0;
}

int MeshBinder::setCullMode(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    mesh->setCullMode((ShaderEngine::CullMode)(luaL_checkinteger(L,2)&3));

    return 0;
}

int MeshBinder::setTexture(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    TextureBase* textureBase = static_cast<TextureBase*>(binder.getInstance("TextureBase", 2));
    int slot=luaL_optinteger(L,3,0);

    mesh->setTexture(textureBase,slot);

    return 0;
}

int MeshBinder::clearTexture(lua_State *L)
{
    Binder binder(L);
    GMesh *mesh = static_cast<GMesh*>(binder.getInstance("Mesh", 1));
    int slot=luaL_optinteger(L,2,0);

    mesh->clearTexture(slot);

    return 0;
}
