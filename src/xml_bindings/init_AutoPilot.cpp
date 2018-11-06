
#include "sflight/xml_bindings/init_AutoPilot.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/AutoPilot.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"

#include <cmath>

namespace sflight {
namespace xml_bindings {

void init_AutoPilot(xml::Node* node, mdls::AutoPilot* ap)
{
   xml::Node* apProps{node->getChild("AutoPilot")};

   std::vector<xml::Node*> comps{xml::getList(apProps, "Component")};

   for (std::size_t i = 0; i < comps.size(); i++) {
      xml::Node* tmp{comps[i]};

      if (xml::get(tmp, "Type", "") == "HeadingHold") {
         ap->player->autoPilotCmds.setMaxBank(
             mdls::UnitConvert::toRads(xml::getDouble(tmp, "MaxBank", 30.0)));
         ap->kphi = xml::getDouble(tmp, "BankWeight", ap->kphi);
         ap->maxBankRate = mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxBankRate", ap->maxBankRate));
         ap->turnType = xml::get(tmp, "TurnType", "").find("TRAJECTORY") == 0
                                   ? mdls::AutoPilot::TurnType::TRAJECTORY
                                   : mdls::AutoPilot::TurnType::HDG;
      } else if (xml::get(tmp, "Type", "") == "AltitudeHold") {
         ap->player->autoPilotCmds.setMaxVS(
             mdls::UnitConvert::FPMtoMPS(xml::getDouble(tmp, "MaxVS", 0.0)));
         ap->kalt = xml::getDouble(tmp, "AltWeight", 0.2);
      } else if (xml::get(tmp, "Type", "") == "VSHold") {
         ap->maxG =
             (xml::getDouble(tmp, "MaxG", ap->maxG) - 1.0) * mdls::nav::getG(0, 0, 0);
         ap->minG =
             (xml::getDouble(tmp, "MinG", ap->minG) - 1.0) * mdls::nav::getG(0, 0, 0);
         ap->maxG_rate = ap->maxG;
         ap->minG_rate = ap->minG;
         ap->player->autoPilotCmds.setMaxPitchUp(mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxPitchUp", mdls::math::PI / 2.0)));
         ap->player->autoPilotCmds.setMaxPitchDown(mdls::UnitConvert::toRads(
             xml::getDouble(tmp, "MaxPitchDown", -mdls::math::PI / 2.0)));
         ap->kpitch = xml::getDouble(tmp, "PitchWeight", 0.0);
      } else if (xml::get(tmp, "Type", "") == "AutoThrottle") {
         ap->maxThrottle = xml::getDouble(tmp, "MaxThrottle", ap->maxThrottle);
         ap->minThrottle = xml::getDouble(tmp, "MinThrottle", ap->minThrottle);
         ap->spoolTime = xml::getDouble(tmp, "SpoolTime", ap->spoolTime);
      }
   }

   ap->hdgErrTol = mdls::UnitConvert::toRads(2);
   ap->player->autoPilotCmds.setAutoPilotOn(true);
   ap->player->autoPilotCmds.setAutoThrottleOn(true);
}
}
}
