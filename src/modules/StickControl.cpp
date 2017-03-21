
#include "modules/StickControl.hpp"

#include "modules/Atmosphere.hpp"

#include "FDMGlobals.hpp"
#include "UnitConvert.hpp"
#include "xml/node_utils.hpp"

namespace sf
{

StickControl::StickControl(FDMGlobals *globals, const double frameRate)
    : FDMModule(globals, frameRate)
{
}

StickControl::~StickControl() {}

void StickControl::initialize(Node *node)
{
   Node* cntrlNode = node->getChild("Control");

   double designAlt = UnitConvert::toMeters(getDouble(cntrlNode, "DesignAltitude", 0));

   double designSpeed = UnitConvert::toMPS(getDouble(cntrlNode, "DesignAirspeed", 0));
   if (designSpeed == 0)
      designSpeed = getDouble(cntrlNode, "DesignPoint/Mach", 0) *
                    Atmosphere::getSpeedSound(Atmosphere::getTemp(designAlt));

   designQbar = 0.5 * Atmosphere::getRho(designAlt) * designSpeed * designSpeed;

   elevGain = getDouble(cntrlNode, "ElevatorGain", 20);
   ailGain = getDouble(cntrlNode, "AileronGain", 50);
   rudGain = getDouble(cntrlNode, "RudderGain", 20);

   pitchGain = getDouble(cntrlNode, "PitchGain", 0);

   elevGain = UnitConvert::toRads(elevGain);
   ailGain = UnitConvert::toRads(ailGain);
   rudGain = UnitConvert::toRads(rudGain);
}

void StickControl::update(double timestep)
{

   if (globals->autoPilotCmds.isAutoPilotOn())
      return;

   double qbar = 0.5 * globals->vInf * globals->vInf * globals->rho;

   double qRatio = 0.0;
   if (designQbar > 0)
   {
      qRatio = qbar / designQbar * cos(globals->alpha);
   }

   //std::cout << "qRatio: " << qRatio << std::endl;

   double desPitch = elevGain * -globals->deflections.get1() * qbar / designQbar *
                     cos(globals->eulers.getPhi());

   double pitchMom = (1.0 - qRatio) * pitchGain * cos(globals->eulers.getTheta()) * cos(globals->eulers.getPhi());
   double yawMom = (1.0 - qRatio) * pitchGain * cos(globals->eulers.getTheta()) * sin(globals->eulers.getPhi());

   globals->pqrdot.set1(globals->deflections.get2() * ailGain * qRatio - globals->pqr.get1());
   globals->pqrdot.set2(globals->deflections.get1() * elevGain * qRatio + pitchMom - globals->pqr.get2());
   globals->pqrdot.set3(globals->deflections.get3() * rudGain * qRatio - yawMom - globals->pqr.get3());
}

}
