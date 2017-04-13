
#include "sflight/fdm/modules/Module.hpp"

#include "sflight/fdm/FDMGlobals.hpp"

namespace sflight {
namespace fdm {

Module::Module(FDMGlobals* globals, const double frameRate)
    : globals(globals)
{
   if (globals != nullptr)
      globals->addModule(this);

   if (frameRate <= 0) {
      frameTime = 0;
   } else {
      frameTime = 1.0 / frameRate;
   }
}
}
}
