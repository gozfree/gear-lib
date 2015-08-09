
CC	= gcc
CXX	= g++
LD	= ld
AR	= ar

COLOR	= 1
CC_V   = $(CC_$(COLOR))
CXX_V  = $(CXX_$(COLOR))
LD_V   = $(LD_$(COLOR))
AR_V   = $(AR_$(COLOR))
CYAN   = "\033[36m"
GREEN  = "\033[1;32m"
NC     = "\033[0m"
AA     = "\033[33m"
CC_0 = $(CC)
CC_1 = @printf '\t%b\t%b\n' $(CYAN)CC$(NC) $(CYAN)$@$(NC); $(CC)
CXX_0 = $(CXX)
CXX_1 = @printf '\t%b\t%b\n' $(CYAN)CXX$(NC) $(CYAN)$@$(NC); $(CXX)
LD_0 = $(LD)
LD_1 = @printf '\t%b\t%b\n' $(GREEN)LD$(NC) $(GREEN)$@$(NC); $(LD)
AR_0 = $(AR)
AR_1 = @printf '\t%b\t%b\n' $(AA)AR$(NC) $(AA)$@$(NC); $(AR)


CFLAGS	:= -g -Wall -fPIC -fno-builtin
CFLAGS	+= -pedantic #-ansi #standard ANSIC
CFLAGS  += -Wno-variadic-macros -Wno-pedantic
CFLAGS  += -I../
#CFLAGS	+= -Werror
#CFLAGS	+= -DLOG_VERBOSE_ENABLE
CFLAGS	+= -DLOG_IO_OPS
LDFLAGS	:= -lpthread
LDFLAGS	+= -lprotobuf
LDFLAGS	+= -llog -lgevent -lskt

.PHONY : all clean

TGT_LIB_A	= ../librpc.a
TGT_UNIT_TEST	= test_proto

TGT	:= $(TGT_UNIT_TEST)

SRC	:= $(wildcard *.c)
SRC	+= $(wildcard *.cc)
SRC	+= $(wildcard *.cpp)
OBJS	:= $(patsubst %.c,%.o,$(SRC))
OBJS	+= $(patsubst %.cc,%.o,$(SRC))
OBJS	+= $(patsubst %.cpp,%.o,$(SRC))

OBJS_UNIT_TEST	= proto_librpc.o ../librpc.o \
                  librpc.pb.cc \
                  hello.pb.cc

all: autogen $(TGT)

autogen:
ifneq (librpc.pb.h, $(wildcard librpc.pb.h))
	protoc -I=./ --cpp_out=./ \
		./librpc.proto \
		./hello.proto
endif

%.o:%.c
	$(CXX_V) -c $(CFLAGS) $< -o $@

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CXX_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

install:

clean:
	rm -f *.o *.pb.h *.pb.cc
	rm -f $(TGT)
