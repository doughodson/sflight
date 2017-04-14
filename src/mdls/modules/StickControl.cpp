
#include "sflight/mdls/modules/StickControl.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

namespace sflight {
namespace mdls {

StickControl::StickControl(Player* player, const double frameRate)
    : Module(player, frameRate)
{
}

void StickControl::initialize(xml::Node* node)
{
   xml::Node* cntrlNode = node->getChild("Control");

   double designAlt =
       UnitConvert::toMeters(xml::getDouble(cntrlNode, "DesignAltitude", 0));

   double designSpeed =
       UnitConvert::toMPS(xml::getDouble(cntrlNode, "DesignAirspeed", 0));
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

void StickControl::update(const double timestep)
{
   if (player->autoPilotCmds.isAutoPilotOn())
      return;

   double qbar = 0.5 * player->vInf * player->vInf * player->rho;

   double qRatio = 0.0;
   if (designQbar > 0) {
      qRatio = qbar / designQbar * cos(player->alpha);
   }

   // std::cout << "qRatio: " << qRatio << std::endl;

   double desPitch = elevGain * -player->deflections.get1() * qbar /
                     designQbar * std::cos(player->eulers.getPhi());

   double pitchMom = (1.0 - qRatio) * pitchGain *
                     std::cos(player->eulers.getTheta()) *
                     std::cos(player->eulers.getPhi());
   double yawMom = (1.0 - qRatio) * pitchGain *
                   std::cos(player->eulers.getTheta()) *
                   std::sin(player->eulers.getPhi());

   player->pqrdot.set1(player->deflections.get2() * ailGain * qRatio -
                        player->pqr.get1());
   player->pqrdot.set2(player->deflections.get1() * elevGain * qRatio +
                        pitchMom - player->pqr.get2());
   player->pqrdot.set3(player->deflections.get3() * rudGain * qRatio -
                        yawMom - player->pqr.get3());
}
}
}
