
#include "modules/FDMModule.hpp"

#include "FDMGlobals.hpp"

namespace sf
{

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

FDMModule::~FDMModule()
{
}
}
