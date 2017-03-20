
include ../makedefs

LIB = ../lib/libsf.a

OBJS =  \
	Atmosphere.o \
	AutoPilotCmds.o \
	Earth.o \
	EOMFiveDOF.o \
	Euler.o \
	FDMGlobals.o \
	FDMModule.o \
	FileOutput.o \
	InterpAero.o \
	InverseDesign.o \
	ModuleLoader.o \
	Node.o \
	NodeUtil.o \
	PIControl.o \
	PID.o \
	PIDControl.o \
	Quaternion.o \
	SimpleAutoPilot.o \
	SimpleEngine.o \
	SimTimer.o \
	StickControl.o \
	Table2D.o \
	Table3D.o \
	TableAero.o \
	Vector3.o \
	Waypoint.o \
	WaypointFollower.o \
	WindAxis.o \
	XMLParser.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f *.o
	-rm -f $(LIB)
