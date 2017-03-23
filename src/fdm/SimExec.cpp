
#include "sflt/fdm/SimExec.hpp"

#include "sflt/xml/Node.hpp"
#include "sflt/xml/node_utils.hpp"

#include "sflt/fdm/FDMGlobals.hpp"

#include <iostream>
#include <thread>
#include <chrono>

//#ifdef _WIN32
//#include "windows.h"
//#else
//#include <unistd.h>
//#endif

namespace sflt {
namespace fdm {

SimExec::SimExec(FDMGlobals* globals, const double frameRate)
    : globals(globals), frameRate(frameRate)
{
}

SimExec::SimExec(FDMGlobals* globals, const double frameRate,
                   const long maxFrames)
    : globals(globals), frameRate(frameRate), maxFrames(maxFrames)
{
}

void SimExec::start()
{
   if (globals == nullptr || frameRate == 0.0)
      return;

   double time{};
   const double frameTime = 1.0 / frameRate;
   const long sleepTime = static_cast<long>(frameTime * 1E3);

   while (globals->frameNum < maxFrames) {
      if (!globals->paused) {
         globals->update(frameTime);
      }

   std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
//#ifdef _WIN32
//      Sleep(sleepTime);
//#else
//      usleep(sleepTime);
//#endif

   }
}

void SimExec::startConstructive()
{
   if (globals == nullptr || frameRate == 0)
      return;

   double time{};
   const double frameTime = 1.0 / frameRate;
   globals->paused = false;
   double frameGroup{};

   while (globals->frameNum < maxFrames) {
      if (frameGroup >= 100) {
         std::cout << "updating frame " << globals->frameNum << " of "
                   << maxFrames << std::endl;
         frameGroup = 0;
      }
      frameGroup++;
      if (!globals->paused) {
         globals->update(frameTime);
      }
   }
}

void SimExec::stop() {}

void SimExec::initialize(xml::Node* node)
{
   frameRate = xml::getDouble(node, "Modules/Rate", 20);
}
}
}
