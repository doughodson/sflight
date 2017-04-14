
#ifndef __sflight_mdls_FileOutput_H__
#define __sflight_mdls_FileOutput_H__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_FileOutput.hpp"
#include "sflight/xml/Node.hpp"

#include <fstream>

namespace sflight {
namespace xml { class Node; }
namespace mdls {
class Player;

//------------------------------------------------------------------------------
// Class: FileOutput
//------------------------------------------------------------------------------
class FileOutput : public Module
{
public:
   FileOutput(Player*, const double frameRate);
   ~FileOutput();

   // module interface
   virtual void update(const double timestep) override;

   void update();
   void close();

   friend void xml_bindings::init_FileOutput(xml::Node*, FileOutput*);

private:
   std::ofstream fout;
   int rate {};
   double lastTime {};
   int frameCounter {};
};

}
}

#endif
