
#ifndef __sflt_fdm_FileOutput_H__
#define __sflt_fdm_FileOutput_H__

#include "sflt/fdm/modules/FDMModule.hpp"

#include <fstream>

namespace sflt {
namespace xml { class Node; }
namespace fdm {
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
   virtual void update(const double timestep) override;

   void update();
   void close();

   //FDMGlobals *globals;
   std::ofstream fout;
   int rate {};
   double lastTime {};
   int frameCounter {};
};

}
}

#endif
