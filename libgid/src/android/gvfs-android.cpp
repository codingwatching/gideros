#include <gvfs-android.h>

#include <stdio.h>
#include <gstdio.h>
#include <string>
#include <algorithm>

#ifdef WINSTORE
#include <io.h>
/* Specifiy one of these flags to define the access mode. */
#define	_O_RDONLY	0
#define _O_WRONLY	1
#define _O_RDWR		2

/* Mask for access mode bits in the _open flags. */
#define _O_ACCMODE	(_O_RDONLY|_O_WRONLY|_O_RDWR)
#define O_ACCMODE	_O_ACCMODE
#else
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include <gpath.h>

#include <map>

#include <gvfs-native.h>

#include <string.h>
#include <ctype.h>
#include "glog.h"
#ifdef __ANDROID__
#include <pystring.h>
#endif
#ifdef _WIN32
#define strcasecmp stricmp
#endif


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct FileInfo
{
	int zipFile;
    size_t startOffset;
    size_t length;
    int encrypt;    // 0 no encryption, 1:encrypt code, 2:encrypt assets
    int drive;
};

static std::vector<std::string> s_zipFiles;
static std::map<std::string, FileInfo> s_files;
static std::map<int, FileInfo> s_fileInfos;
static std::string s_zipFile;
static bool s_playerModeEnabled = false;

static char s_codeKey[256] = {0};
static char s_assetsKey[256] = {0};

static int s_open(const char *pathname, int flags)
{
	int drive = gpath_getPathDrive(pathname);
	
	if (drive != GPATH_ABSOLUTE)
    {
        if ((gpath_getDriveFlags(drive) & GPATH_RO) == GPATH_RO && (flags & O_ACCMODE) != O_RDONLY)
        {
            errno = EACCES;
            return -1;
        }
    }
	
    int fd = -1;
    
    FileInfo fi = {-1, (size_t)-1, (size_t)-1, 0, drive};
    
    int local= 0;
#ifdef __EMSCRIPTEN__
    local=EM_ASM_INT({ return Module.requestFile(UTF8ToString($0)); },pathname);
#endif
#ifdef __ANDROID__
	local = s_playerModeEnabled;
#else
	if (s_zipFile.empty()) local=true;
#endif
	
    if ( drive != 0 || local )
    {
        fd=::open(gpath_transform(pathname), flags, 0755);
        //glog_d("Opened %s(%s) at fd %d on drive %d\n",pathname,gpath_transform(pathname),fd,drive);
    }
    else
    {
#ifdef __ANDROID__
	std::string normpathname = pystring::os::path::normpath(gpath_transform(pathname));
	pathname = normpathname.c_str();
#else	
    	pathname = gpath_normalizeArchivePath(pathname);
    	//glog_d("Looking for %s in archive %s",pathname,s_zipFile.c_str());
#endif
    std::map<std::string, FileInfo>::iterator iter;
    iter = s_files.find(pathname);

    if (iter == s_files.end())
    {
        //glog_d("%s Not found in archive",pathname);
        errno = ENOENT;
        return -1;
    }

    if ((flags & O_ACCMODE) != O_RDONLY)
    {
        errno = EACCES;
        return -1;
    }
    const char *zip;
#ifdef __ANDROID__
	zip=s_zipFiles[iter->second.zipFile].c_str();
#else
	zip=s_zipFile.c_str();
#endif	

    	fd = ::open(zip, flags, 0755);
    	//glog_d("%s: fd is %d",pathname,fd);

        ::lseek(fd, iter->second.startOffset, SEEK_SET);

        fi = iter->second;
	}
    if (fd < 0)
        return fd;

    if (drive == 0)
    {
    	//Probe encryption
   		unsigned char cryptsig[4];
   		int rdc=0;
   		off_t rlength=0;
   		if (fi.length==((size_t)-1))
   		{
       		rlength=::lseek(fd, -4, SEEK_END);
       		rdc=::read(fd, cryptsig, 4);
            ::lseek(fd, 0, SEEK_SET);
  		}
   		else
    	{
    		::lseek(fd, fi.startOffset+fi.length-4, SEEK_SET);
       		rdc=::read(fd, cryptsig, 4);
            ::lseek(fd, fi.startOffset, SEEK_SET);
            rlength=fi.length-4;
  		}
   		if (rdc==4)
   		{
       		cryptsig[0]^=(rlength>>24)&0xFF;
       		cryptsig[1]^=(rlength>>16)&0xFF;
       		cryptsig[2]^=(rlength>>8)&0xFF;
       		cryptsig[3]^=(rlength>>0)&0xFF;
    		if ((cryptsig[0]=='G')&&(cryptsig[1]=='x')&&(cryptsig[2]==0xE7))
    		{
    			if ((cryptsig[3]==1)||(cryptsig[3]==2))
    			{
       				   if (fi.length==((size_t)-1))
       				    	fi.startOffset=0;
                    fi.encrypt = cryptsig[3];
                       fi.length=rlength;
#ifndef __ANDROID__                       
                       fi.drive=drive;
#endif                       
    			}
    		}
    	}
    }

    s_fileInfos[fd] = fi;

    return fd;
}

#ifdef EMSCRIPTEN
extern void flushDrive(int drive);
#endif
static int s_close(int fd)
{
    std::map<int, FileInfo>::iterator iter;
    iter = s_fileInfos.find(fd);
    
	if (iter == s_fileInfos.end()) /* sanity check */
    {
        errno = EBADF;
        return -1;
    }
	
    int drive=iter->second.drive;
    s_fileInfos.erase(fd);
    int cret=close(fd);
#ifdef EMSCRIPTEN
    flushDrive(drive);
#endif
    return cret;
}

static size_t readHelper(int fd, void* buf, size_t count)
{
    std::map<int, FileInfo>::iterator iter;
    iter = s_fileInfos.find(fd);

    if (iter == s_fileInfos.end()) /* sanity check */
    {
        errno = EBADF;
        return -1;
    }

    if (iter->second.startOffset == (size_t)-1 && iter->second.length == (size_t)-1)
        return ::read(fd, buf, count);

    size_t endOffset = iter->second.startOffset + iter->second.length;

    size_t curr = ::lseek(fd, 0, SEEK_CUR);

    if (curr < iter->second.startOffset || curr >= endOffset)
        return 0;

    size_t rem = endOffset - curr;

    return ::read(fd, buf, std::min(rem, count));
}

static size_t s_write(int fd, const void* buf, size_t count)
{
    std::map<int, FileInfo>::iterator iter;
    iter = s_fileInfos.find(fd);

    if (iter == s_fileInfos.end()) /* sanity check */
    {
        errno = EBADF;
        return -1;
    }

    if (iter->second.startOffset != (size_t)-1 || iter->second.length != (size_t)-1) /* sanity check */
    {
        errno = EACCES;
        return -1;
    }

    return ::write(fd, buf, count);
}

static off_t s_lseek(int fd, off_t offset, int whence)
{
    std::map<int, FileInfo>::iterator iter;
    iter = s_fileInfos.find(fd);

    if (iter == s_fileInfos.end()) /* sanity check */
    {
        errno = EBADF;
        return -1;
    }

    if (iter->second.startOffset == (size_t)-1 && iter->second.length == (size_t)-1)
        return ::lseek(fd, offset, whence);

    size_t startOffset = iter->second.startOffset;
    size_t endOffset = startOffset + iter->second.length;

    switch (whence)
    {
    case SEEK_SET:
    {
        off_t result = ::lseek(fd, startOffset + offset, SEEK_SET);
        return result - startOffset;
    }
    case SEEK_CUR:
    {
        off_t result = ::lseek(fd, offset, SEEK_CUR);
        return result - startOffset;
    }
    case SEEK_END:
    {
        off_t result = ::lseek(fd, endOffset + offset, SEEK_SET);
        return result - startOffset;
    }
    }

    errno = EINVAL;
    return -1;
}

static size_t s_read(int fd, void* buf, size_t count)
{
    std::map<int, FileInfo>::iterator iter;
    iter = s_fileInfos.find(fd);

    if (iter == s_fileInfos.end()) /* sanity check */
    {
        errno = EBADF;
        return -1;
    }

    size_t size;

    char *key=NULL;
    switch (iter->second.encrypt)
    {
    case 0:
        key = NULL;
        break;
    case 1:
        key = s_codeKey;
        break;
    case 2:
        key = s_assetsKey;
        break;
    }

    if (key)
    {
        off_t curr = s_lseek(fd, 0, SEEK_CUR);
        size = readHelper(fd, buf, count);

        if (curr != (off_t)-1 && size != (size_t)-1)
            for (size_t i = (curr<32)?(32-curr):0; i < size; ++i)
                ((char*)buf)[i] ^= key[(((curr+i)*13)+(((curr+i)/256)*31))%256];
    }
    else
    {
        size = readHelper(fd, buf, count);
    }

    return size;
}

static const char *codeKey_   = "312e68c04c6fd22922b5b232ea6fb3e1"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		;
static const char *assetsKey_ = "312e68c04c6fd22922b5b232ea6fb3e2"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		;

extern "C" {

void gvfs_init()
{
	g_Vfs vfs =
	{
		s_open,
		s_close,
		s_read,
		s_write,
		s_lseek,
		s_lflags,
	};

	g_setVfs(vfs);
    
    gvfs_setCodeKey(codeKey_ + 32);
	gvfs_setAssetsKey(assetsKey_ + 32);
}

void gvfs_cleanup()
{
	std::map<int, FileInfo>::iterator iter, e = s_fileInfos.end();
	for (iter = s_fileInfos.begin(); iter != e; ++iter)
		::close(iter->first);

	s_zipFiles.clear();
	s_files.clear();
	s_fileInfos.clear();
	s_playerModeEnabled = false;

	static g_Vfs nullvfs =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	};

	g_setVfs(nullvfs);
}

#ifdef __ANDROID__
void gvfs_setPlayerModeEnabled(int playerMode)
{
    s_playerModeEnabled = playerMode;
}

int gvfs_isPlayerModeEnabled()
{
    return s_playerModeEnabled;
}

void gvfs_setZipFiles(const char *apkFile, const char *mainFile, const char *patchFile)
{
	s_zipFiles.clear();
    s_zipFiles.push_back(apkFile);
	s_zipFiles.push_back(mainFile);
	s_zipFiles.push_back(patchFile);
}
#endif

void gvfs_addFile(const char *pathname, int zipFile, size_t startOffset, size_t length)
{
    FileInfo f = {zipFile, startOffset, length, false,0};
    s_files[pathname] = f;
}

void gvfs_setCodeKey(const char key[256])
{
    memcpy(s_codeKey, key, 256);
}

void gvfs_setAssetsKey(const char key[256])
{
    memcpy(s_assetsKey, key, 256);
}

}
