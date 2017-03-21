
#include "sf/fdm/modules/FDMModule.hpp"

#include "sf/fdm/FDMGlobals.hpp"

namespace sf {
namespace fdm {

FDMModule::FDMModule(FDMGlobals *globals, double frameRate)
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
