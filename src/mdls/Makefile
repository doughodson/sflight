
include ../../makedefs

LIB = ../../lib/libsflight_mdls.a

OBJS =  \
	modules/AutoPilot.o \
	modules/Atmosphere.o \
	modules/Engine.o \
	modules/EOMFiveDOF.o \
	modules/FileOutput.o \
	modules/InterpAero.o \
	modules/InverseDesign.o \
	modules/Module.o \
	modules/StickControl.o \
	modules/TableAero.o \
	modules/WaypointFollower.o \
	AutoPilotCmds.o \
	Euler.o \
	nav_utils.o \
	Player.o \
	PIControl.o \
	PID.o \
	PIDControl.o \
	Quaternion.o \
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
