
include ../../makedefs

LIB = ../../lib/libsflight_xml_bindings.a

OBJS =  \
	init_AutoPilot.o \
	load_modules.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f *.o
	-rm -f $(LIB)
