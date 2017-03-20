
#ifndef __FDMModule_H__
#define __FDMModule_H__

#include <string>

namespace sf {
class FDMGlobals;
class Node;

//------------------------------------------------------------------------------
// Class: FDMModule
// Description: Base class for all flight dynamics modules
//------------------------------------------------------------------------------
class FDMModule
{
public:
   FDMModule();
   FDMModule(FDMGlobals *globals, double frameRate);
   virtual ~FDMModule();

   virtual void update(double timestep);
   virtual void initialize(Node* node);

   virtual void setProperty(std::string tag, double val);

   FDMGlobals* globals {};

   double frameTime {};
   double lastTime {};
};
    
}

#endif
