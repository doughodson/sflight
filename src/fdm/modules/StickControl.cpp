
#include "fdm/modules/StickControl.hpp"

#include "fdm/modules/Atmosphere.hpp"

#include "xml/Node.hpp"
#include "xml/node_utils.hpp"

#include "fdm/FDMGlobals.hpp"
#include "fdm/UnitConvert.hpp"

namespace sf
{

StickControl::StickControl(FDMGlobals *globals, const double frameRate)
    : FDMModule(globals, frameRate)
{
}

void StickControl::initialize(xml::Node *node)
{
   xml::Node* cntrlNode = node->getChild("Control");

   double designAlt = UnitConvert::toMeters(xml::getDouble(cntrlNode, "DesignAltitude", 0));

   double designSpeed = UnitConvert::toMPS(xml::getDouble(cntrlNode, "DesignAirspeed", 0));
   if (designSpeed == 0)
      designSpeed = xml::getDouble(cntrlNode, "DesignPoint/Mach", 0) *
                    Atmosphere::getSpeedSound(Atmosphere::getTemp(designAlt));

   designQbar = 0.5 * Atmosphere::getRho(designAlt) * designSpeed * designSpeed;

   elevGain = xml::getDouble(cntrlNode, "ElevatorGain", 20);
   ailGain = xml::getDouble(cntrlNode, "AileronGain", 50);
   rudGain = xml::getDouble(cntrlNode, "RudderGain", 20);

   pitchGain = xml::getDouble(cntrlNode, "PitchGain", 0);

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
