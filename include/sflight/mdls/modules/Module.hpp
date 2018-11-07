
#ifndef __sflight_mdls_Module_HPP__
#define __sflight_mdls_Module_HPP__

#include <string>

namespace sflight {
namespace xml {
class Node;
}
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: Module
// Description: Base class for all modules
//------------------------------------------------------------------------------
class Module
{
 public:
   Module(Player*, const double frameRate);
   virtual ~Module() = default;

   // module interface
   virtual void update(const double timestep){};

   Player* player{};

   double frameTime{};
   double lastTime{};
};
}
}

#endif
