LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := gvfs

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS := -O2

LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. $(LOCAL_PATH)/../private
	
LOCAL_SRC_FILES += \
    ../wsetup.c \
    ../wbuf.c \
    ../vfscanf.c \
    ../vfprintf.c \
    ../ungetc.c \
    ../tmpfile.c \
    ../stdio.c \
    ../setvbuf.c \
    ../rget.c \
    ../refill.c \
    ../putc.c \
    ../makebuf.c \
    ../getc.c \
    ../fwrite.c \
    ../fwalk.c \
    ../fvwrite.c \
    ../ftell.c \
    ../fseek.c \
    ../fscanf.c \
    ../freopen.c \
    ../fread.c \
    ../fputs.c \
    ../fputc.c \
    ../fprintf.c \
    ../fopen.c \
    ../flockfile.c \
    ../flags.c \
    ../findfp.c \
    ../fileops.c \
    ../fgets.c \
    ../fgetc.c \
    ../fflush.c \
    ../ferror.c \
    ../feof.c \
    ../fclose.c \
    ../extra.c \
    ../clrerr.c \
    ../gfile.cpp \
    ../gpath.cpp
	
LOCAL_LDLIBS := -ldl

include $(BUILD_SHARED_LIBRARY)
