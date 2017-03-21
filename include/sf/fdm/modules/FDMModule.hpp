
#ifndef __FDMModule_H__
#define __FDMModule_H__

#include <string>

namespace sf
{
namespace xml { class Node; }
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: FDMModule
// Description: Base class for all flight dynamics modules
//------------------------------------------------------------------------------
class FDMModule
{
 public:
   FDMModule();
   FDMModule(FDMGlobals* globals, double frameRate);
   virtual ~FDMModule();

   // module interface
   virtual void initialize(xml::Node *node)               {};
   virtual void update(double timestep)                   {};
   virtual void setProperty(std::string tag, double val)  {};

   FDMGlobals* globals {};

   double frameTime {};
   double lastTime {};
};
}

#endif
