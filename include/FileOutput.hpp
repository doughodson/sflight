
#ifndef FileOutput_H
#define FileOutput_H

#include "FDMModule.hpp"
#include <fstream>

namespace sf
{
class FDMGlobals;
class Node;

//------------------------------------------------------------------------------
// Class: FileOutput
//------------------------------------------------------------------------------
class FileOutput : public virtual FDMModule
{
public:
   FileOutput(FDMGlobals* globals, double frameRate);
   ~FileOutput();

   virtual void initialize(Node* node) override;
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
