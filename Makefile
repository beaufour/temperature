MONO_PATH=/usr/local/openmono
include $(MONO_PATH)/predefines.mk

TARGET=temperature

OBJECTS = \
$(patsubst %.c,%.o,$(wildcard lib/*.c)) \
$(patsubst %.cpp,%.o,$(wildcard lib/*.cpp)) \
$(patsubst %.cpp,%.o,$(wildcard *.cpp))

OPTIMIZATION = -Os -Wno-unused-function

include $(MONO_PATH)/mono.mk

-include Makefile.local
