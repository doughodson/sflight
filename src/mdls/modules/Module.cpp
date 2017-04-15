
#include "sflight/mdls/modules/Module.hpp"

#include "sflight/mdls/Player.hpp"

namespace sflight {
namespace mdls {

Module::Module(Player* p, const double frameRate)
    : player(p)
{
   if (frameRate <= 0) {
      frameTime = 0;
   } else {
      frameTime = 1.0 / frameRate;
   }
}
}
}
