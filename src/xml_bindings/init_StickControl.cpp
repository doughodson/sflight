
#include "sflight/xml_bindings/init_StickControl.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/modules/Atmosphere.hpp"
#include "sflight/mdls/modules/StickControl.hpp"

#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/constants.hpp"

#include <cmath>
#include <iostream>

namespace sflight {
namespace xml_bindings {

void init_StickControl(xml::Node* node, mdls::StickControl* sc)
{
   std::cout << std::endl;
   std::cout << "-------------------------" << std::endl;
   std::cout << "Module: StickControl"      << std::endl;
   std::cout << "-------------------------" << std::endl;

   xml::Node* cntrlNode{node->getChild("Control")};

   double designAlt{mdls::UnitConvert::toMeters(xml::getDouble(cntrlNode, "DesignAltitude", 0.0))};

   double designSpeed{mdls::UnitConvert::toMPS(xml::getDouble(cntrlNode, "DesignAirspeed", 0.0))};
   if (designSpeed == 0.0) {
      designSpeed = xml::getDouble(cntrlNode, "DesignPoint/Mach", 0.0) *
                    mdls::Atmosphere::getSpeedSound(mdls::Atmosphere::getTemp(designAlt));
   }

   sc->designQbar = 0.5 * mdls::Atmosphere::getRho(designAlt) * designSpeed * designSpeed;

   sc->elevGain = xml::getDouble(cntrlNode, "ElevatorGain", 20.0);
   sc->ailGain = xml::getDouble(cntrlNode, "AileronGain", 50.0);
   sc->rudGain = xml::getDouble(cntrlNode, "RudderGain", 20.0);

   sc->pitchGain = xml::getDouble(cntrlNode, "PitchGain", 0.0);

   sc->elevGain = mdls::UnitConvert::toRads(sc->elevGain);
   sc->ailGain = mdls::UnitConvert::toRads(sc->ailGain);
   sc->rudGain = mdls::UnitConvert::toRads(sc->rudGain);

   std::cout << "-------------------------" << std::endl;
}
}
}
