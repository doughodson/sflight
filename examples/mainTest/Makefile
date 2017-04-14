#
include ../../makedefs

PGM = mainTest

OBJS = main.o SimExec.o

LDFLAGS += -lsflight_mdls -lsflight_xml
#LDFLAGS += -lftgl -lfreetype -lglut -lGLU -lGL -lX11 -lpthread
#LDFLAGS += -lXi -lXmu
#LDFLAGS += -lrt

all: $(PGM)

$(PGM): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-rm -f *.o
	-rm -f $(PGM)
