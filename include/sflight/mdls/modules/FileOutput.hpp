
#ifndef __sflight_mdls_FileOutput_HPP__
#define __sflight_mdls_FileOutput_HPP__

#include "sflight/mdls/modules/Module.hpp"

#include "sflight/xml_bindings/init_FileOutput.hpp"

#include <fstream>

namespace sflight {
namespace xml {
class Node;
}
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

   friend void xml_bindings::init_FileOutput(xml::Node*, FileOutput*);

 private:
   void update();

   std::ofstream fout;
   int rate{};
   double lastTime{};
   int frameCounter{};
};
}
}

#endif
