
#ifndef FileOutput_H
#define FileOutput_H

#include "sf/fdm/modules/FDMModule.hpp"

#include <fstream>

namespace sf {
namespace xml { class Node; }
class FDMGlobals;

//------------------------------------------------------------------------------
// Class: FileOutput
//------------------------------------------------------------------------------
class FileOutput : public virtual FDMModule
{
public:
   FileOutput(FDMGlobals* globals, double frameRate);
   ~FileOutput();

   // module interface
   virtual void initialize(xml::Node* node) override;
   virtual void update(double timestep) override;

   void update();
   void close();

   //FDMGlobals *globals;
   std::ofstream fout;
   int rate {};
   double lastTime {};
   int frameCounter {};
};

}

#endif
