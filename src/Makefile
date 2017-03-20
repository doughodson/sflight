
include ../makedefs

LIB = ../lib/libsf.a

OBJS =  \
	Atmosphere.o \
	AutoPilotCmds.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f *.o
	-rm -f $(LIB)
