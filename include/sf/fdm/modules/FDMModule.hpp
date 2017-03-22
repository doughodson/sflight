
#ifndef __FDMModule_H__
#define __FDMModule_H__

#include <string>

namespace sf {
namespace xml {
class Node;
}
namespace fdm {
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: FDMModule
// Description: Base class for all flight dynamics modules
//------------------------------------------------------------------------------
class FDMModule
{
 public:
   FDMModule(FDMGlobals* globals, const double frameRate);
   virtual ~FDMModule() = default;

   // module interface
   virtual void initialize(xml::Node* node){};
   virtual void update(const double timestep){};
   virtual void setProperty(std::string tag, const double val){};

   FDMGlobals* globals{};

   double frameTime{};
   double lastTime{};
};
}
}

#endif
