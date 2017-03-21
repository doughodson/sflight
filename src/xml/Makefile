
include ../../makedefs

LIB = ../../lib/libxml.a

OBJS =  \
	Node.o \
	node_utils.o \
	parser_utils.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f *.o
	-rm -f $(LIB)
