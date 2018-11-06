
#include "sflight/mdls/Player.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"
#include "sflight/mdls/modules/Module.hpp"

#include "sflight/mdls/AutoPilotCmds.hpp"
#include "sflight/mdls/Euler.hpp"
#include "sflight/mdls/Quaternion.hpp"
#include "sflight/mdls/UnitConvert.hpp"
#include "sflight/mdls/Vector3.hpp"

#include "sflight/mdls/nav_utils.hpp"
#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include <cmath>
#include <string>

namespace sflight {
namespace mdls {

Player::Player() { g = nav::getG(0, 0, 0); }

Player::~Player()
{
   for (std::size_t i = 0; i < modules.size(); i++) {
      delete modules[i];
   }
}

void Player::addModule(Module* module) { this->modules.push_back(module); }

void Player::update(double timestep)
{
   simTime += timestep;

   for (unsigned int i = 0; i < modules.size(); i++) {
      timestep = simTime - modules[i]->lastTime;
      if (timestep >= modules[i]->frameTime) {
         modules[i]->lastTime = simTime;
         modules[i]->update(timestep);
      }
   }
   frameNum++;
}
}
}
