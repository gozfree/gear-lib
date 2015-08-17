
OUTPUT		?= /usr/local

ARCH		?=x86
CROSS_PREFIX	?=

ifeq ($(ARCH), pi)
	CROSS_PREFIX=arm-linux-gnueabihf-
endif

CC	= ${CROSS_PREFIX}gcc
CXX	= ${CROSS_PREFIX}g++
LD	= ${CROSS_PREFIX}ld
AR	= ${CROSS_PREFIX}ar

COLOR	= 1
CC_V	= $(CC_$(COLOR))
CXX_V	= $(CXX_$(COLOR))
LD_V	= $(LD_$(COLOR))
AR_V	= $(AR_$(COLOR))
CP_V	= $(CP_$(COLOR))
RM_V	= $(RM_$(COLOR))
CYAN	= "\033[36m"
GREEN	= "\033[1;32m"
NC	= "\033[0m"
AA	= "\033[33m"
CC_0	= $(CC)
CC_1	= @printf '\t%b\t%b\n' $(CYAN)CC$(NC) $(CYAN)$@$(NC); $(CC)
CXX_0	= $(CXX)
CXX_1	= @printf '\t%b\t%b\n' $(CYAN)CXX$(NC) $(CYAN)$@$(NC); $(CXX)
LD_0	= $(LD)
LD_1	= @printf '\t%b\t%b\n' $(GREEN)LD$(NC) $(GREEN)$@$(NC); $(LD)
AR_0	= $(AR)
AR_1	= @printf '\t%b\t%b\n' $(AA)AR$(NC) $(AA)$@$(NC); $(AR)
CP_0	= cp
CP_1	= @printf '\t%b\n' $(AA)install$(NC); cp
RM_0	= rm
RM_1	= @printf '\t%b\n' $(AA)clean$(NC); rm

########
LIBNAME		= libjpeg-ex
TGT_LIB_H	= ${LIBNAME}.h
TGT_LIB_A	= ${LIBNAME}.a
TGT_LIB_SO	= ${LIBNAME}.so
TGT_UNIT_TEST	= test_${LIBNAME}

OBJS_LIB	= ${LIBNAME}.o
ifeq ($(ARCH), pi)
OBJS_LIB	+= chrome_convert.o
endif
OBJS_UNIT_TEST	= test_${LIBNAME}.o

CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= -I$(OUTPUT)/include
SHARED	:= -shared
LDFLAGS	:= -lpthread
LDFLAGS	+= -ljpeg
LDFLAGS	+= -L$(OUTPUT)/lib
LDFLAGS	+= -llog

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

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)

install:
	$(CP_V) -r $(TGT_LIB_H)  ${OUTPUT}/include
	$(CP_V) -r $(TGT_LIB_A)  ${OUTPUT}/lib
	$(CP_V) -r $(TGT_LIB_SO) ${OUTPUT}/lib

uninstall:
	$(RM_V) -f ${OUTPUT}/include/$(TGT_LIB_H)
	$(RM_V) -f ${OUTPUT}/lib/$(TGT_LIB_A)
	$(RM_V) -f ${OUTPUT}/lib/$(TGT_LIB_SO)
