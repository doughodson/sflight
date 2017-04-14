
#include "sflight/xml_bindings/init_AutoPilot.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"
#include "sflight/mdls/modules/AutoPilot.hpp"

#include <cmath>

namespace sflight {
namespace xml_bindings {

void init_AutoPilot(xml::Node* node)
{
   auto player = new mdls::Player();
   const double frameRate{};
   auto autoPilot = new mdls::AutoPilot(player, frameRate);

   xml::Node* apProps = node->getChild("AutoPilot");

   std::vector<xml::Node*> comps = xml::getList(apProps, "Component");

   for (int i = 0; i < comps.size(); i++) {
      xml::Node* tmp = comps[i];

      if (xml::get(tmp, "Type", "") == "HeadingHold") {
         player->autoPilotCmds.setMaxBank(
             mdls::UnitConvert::toRads(xml::getDouble(tmp, "MaxBank", 30)));
         autoPilot->kphi = xml::getDouble(tmp, "BankWeight", autoPilot->kphi);
         autoPilot->maxBankRate = mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxBankRate", autoPilot->maxBankRate));
         autoPilot->turnType = xml::get(tmp, "TurnType", "").find("TRAJECTORY") == 0
                                   ? mdls::AutoPilot::TurnType::TRAJECTORY
                                   : mdls::AutoPilot::TurnType::HDG;
      } else if (xml::get(tmp, "Type", "") == "AltitudeHold") {
         player->autoPilotCmds.setMaxVS(
             mdls::UnitConvert::FPMtoMPS(xml::getDouble(tmp, "MaxVS", 0)));
         autoPilot->kalt = xml::getDouble(tmp, "AltWeight", 0.2);
      } else if (xml::get(tmp, "Type", "") == "VSHold") {
         autoPilot->maxG =
             (xml::getDouble(tmp, "MaxG", autoPilot->maxG) - 1) * mdls::nav::getG(0, 0, 0);
         autoPilot->minG =
             (xml::getDouble(tmp, "MinG", autoPilot->minG) - 1) * mdls::nav::getG(0, 0, 0);
         autoPilot->maxG_rate = autoPilot->maxG;
         autoPilot->minG_rate = autoPilot->minG;
         player->autoPilotCmds.setMaxPitchUp(mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxPitchUp", mdls::math::PI / 2.0)));
         player->autoPilotCmds.setMaxPitchDown(mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxPitchDown", -mdls::math::PI / 2.0)));
         autoPilot->kpitch = xml::getDouble(tmp, "PitchWeight", 0);
      } else if (xml::get(tmp, "Type", "") == "AutoThrottle") {
         autoPilot->maxThrottle = xml::getDouble(tmp, "MaxThrottle", autoPilot->maxThrottle);
         autoPilot->minThrottle = xml::getDouble(tmp, "MinThrottle", autoPilot->minThrottle);
         autoPilot->spoolTime = xml::getDouble(tmp, "SpoolTime", autoPilot->spoolTime);
      }
   }

   autoPilot->hdgErrTol = mdls::UnitConvert::toRads(2);
   autoPilot->player->autoPilotCmds.setAutoPilotOn(true);
   autoPilot->player->autoPilotCmds.setAutoThrottleOn(true);
}
}
}
