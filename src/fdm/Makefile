
include ../../makedefs

LIB = ../../lib/libsflt_fdm.a

OBJS =  \
	modules/Atmosphere.o \
	modules/EOMFiveDOF.o \
	modules/FDMModule.o \
	modules/FileOutput.o \
	modules/InterpAero.o \
	modules/InverseDesign.o \
	modules/SimpleAutoPilot.o \
	modules/SimpleEngine.o \
	modules/StickControl.o \
	modules/TableAero.o \
	modules/WaypointFollower.o \
	AutoPilotCmds.o \
	Euler.o \
	FDMGlobals.o \
	load_modules.o \
	nav_utils.o \
	PIControl.o \
	PID.o \
	PIDControl.o \
	Quaternion.o \
	SimExec.o \
	Table2D.o \
	Table3D.o \
	Vector3.o \
	Waypoint.o \
	WindAxis.o

.PHONY: all clean

all: $(LIB)

$(LIB) : $(OBJS)
	ar rs $@ $(OBJS)

clean:
	-rm -f modules/*.o
	-rm -f *.o
	-rm -f $(LIB)
