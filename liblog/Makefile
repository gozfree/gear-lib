
#ARCH: x86/pi/android/ios/
ARCH		?=x86
CROSS_PREFIX	?=
OUTPUT		?= /usr/local
ARCH_CONF	:= $(ARCH).conf

ifeq ($(ARCH_CONF), $(wildcard $(ARCH_CONF)))
include $(ARCH_CONF)
endif

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar

include color.conf
#COLOR	0: disable color; 1: enable color
COLOR	= 1
CC_V	= $(CC_$(COLOR))
CXX_V	= $(CXX_$(COLOR))
LD_V	= $(LD_$(COLOR))
AR_V	= $(AR_$(COLOR))
CP_V	= $(CP_$(COLOR))
RM_V	= $(RM_$(COLOR))
CC_0	= $(CC)
CC_1	= @printf '%b\t%b\t%b\n' $(CYAN)$(ARCH) CC $@$(NC); $(CC)
CXX_0	= $(CXX)
CXX_1	= @printf '%b\t%b\t%b\n' $(CYAN)$(ARCH) CXX $@$(NC); $(CXX)
LD_0	= $(LD)
LD_1	= @printf '%b\t%b\t%b\n' $(BGREEN)$(ARCH) LD $@$(NC); $(LD)
AR_0	= $(AR)
AR_1	= @printf '%b\t%b\t%b\n' $(YELLOW)$(ARCH) AR $@$(NC); $(AR)
CP_0	= cp
CP_1	= @printf '%b\t%b\n' $(BGREEN)$(ARCH) install$(NC); cp
RM_0	= rm
RM_1	= @printf '%b\t%b\n' $(BGREEN)$(ARCH) clean$(NC); rm

########
LIBNAME		= liblog
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LIB	= $(LIBNAME).o
OBJS_UNIT_TEST	= test_$(LIBNAME).o

CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include
SHARED	:= -shared
LDFLAGS	:=  $($(ARCH)_LDFLAGS)
LDFLAGS	+= -L$(PLATFORM_LIB) -llog
LDFLAGS	+= -pthread

.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR_V) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(LD_V) -o $@ $^ $(SHARED)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)

install:
	$(MAKEDIR_OUTPUT)
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include
	$(CP_V) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTPUT)/lib

uninstall:
	$(RM_V) -f $(OUTPUT)/include/$(TGT_LIB_H)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
