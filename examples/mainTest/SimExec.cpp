
#include "SimExec.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"

#include <iostream>
#include <thread>
#include <chrono>

SimExec::SimExec(sflight::mdls::Player* p, const double frameRate)
    : player(p), frameRate(frameRate)
{
}

SimExec::SimExec(sflight::mdls::Player* p, const double frameRate,  const std::size_t maxFrames)
    : player(p), frameRate(frameRate), maxFrames(maxFrames)
{
}

void SimExec::start()
{
   if (player == nullptr || frameRate == 0.0)
      return;

   double time{};
   const double frameTime{1.0 / frameRate};
   const long sleepTime{static_cast<long>(frameTime * 1E3)};

   while (player->frameNum < maxFrames) {
      if (!player->paused) {
         player->update(frameTime);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
   }
}

void SimExec::startConstructive()
{
   if (player == nullptr || frameRate == 0)
      return;

   const double frameTime{1.0 / frameRate};
   player->paused = false;
   double frameGroup{};

   while (player->frameNum < maxFrames) {
      if (frameGroup >= 100) {
         std::cout << "updating frame " << player->frameNum << " of "
                   << maxFrames << std::endl;
         frameGroup = 0;
      }
      frameGroup++;
      if (!player->paused) {
         player->update(frameTime);
      }
   }
}

void SimExec::stop() {}

void SimExec::initialize(sflight::xml::Node* node)
{
   frameRate = sflight::xml::getDouble(node, "Modules/Rate", 20.0);
}
