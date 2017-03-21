
#include "modules/FDMModule.hpp"

#include "FDMGlobals.hpp"
#include <string>

namespace sf
{

FDMModule::FDMModule(FDMGlobals *globals, double frameRate)
    : globals(globals)
{
   if (globals != nullptr)
      globals->addModule(this);

   if (frameRate <= 0)
   {
      this->frameTime = 0;
   }
   else
   {
      this->frameTime = 1.0 / frameRate;
   }
}

FDMModule::~FDMModule()
{
}
}
