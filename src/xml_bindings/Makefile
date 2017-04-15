
include ../../makedefs

LIB = ../../lib/libsflight_xml_bindings.a

OBJS =  \
	builder.o \
	init_AutoPilot.o \
	init_Engine.o \
	init_EOMFiveDOF.o \
	init_FileOutput.o \
	init_InterpAero.o \
	init_InverseDesign.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f *.o
	-rm -f $(LIB)
