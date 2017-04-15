
#include "sflight/mdls/modules/AutoPilot.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

AutoPilot::AutoPilot(Player* player, const double frameRate) : Module(player, frameRate) {}

AutoPilot::~AutoPilot() {}

void AutoPilot::update(const double timestep)
{
   if (player->autoPilotCmds.isAutoPilotOn()) {

      // null rotational accel rates ( in case of previous manual control )
      player->pqrdot.multiply(0);

      if (player->autoPilotCmds.isOrbitHoldOn()) {
         updateHdg(timestep, player->eulers.getPsi() +
                                 math::PI / 2.0 *
                                     UnitConvert::signum(player->autoPilotCmds.getMaxBank()));
         updateAlt(timestep);
      }

      if (player->autoPilotCmds.isAltHoldOn()) {
         updateAlt(timestep);
      } else if (player->autoPilotCmds.isVsHoldOn()) {
         updateVS(timestep, player->autoPilotCmds.getCmdVertSpeed());
      }

      if (player->autoPilotCmds.isHdgHoldOn()) {
         updateHdg(timestep, player->autoPilotCmds.getCmdHeading());
      }
   }

   if (player->autoPilotCmds.isAutoThrottleOn()) {
      updateSpeed(timestep);
   }
}

void AutoPilot::updateHdg(double timestep, double cmdHdg)
{
   double hdgDiff{};
   if (turnType == TurnType::TRAJECTORY) {
      hdgDiff = cmdHdg - std::atan2(player->nedVel.get2(), player->nedVel.get1());
   } else {
      hdgDiff = cmdHdg - player->eulers.getPsi();
   }

   // if hdgDiff > 180, turn opp dir by making hdgDiff neg num <180
   hdgDiff = UnitConvert::wrapHeading(hdgDiff, true);

   double phiCmd = kphi * hdgDiff * player->vInf * maxBankRate;
   // bound phiCmd to -maxBank...maxBank
   phiCmd = std::min(player->autoPilotCmds.getMaxBank(),
                     std::max(-player->autoPilotCmds.getMaxBank(), phiCmd));

   // bound pCmd to -maxBankRate...maxBankRate
   const double pCmd =
       std::min(maxBankRate, std::max(-maxBankRate, phiCmd - player->eulers.getPhi()));

   player->pqr.set1(pCmd);
}

void AutoPilot::updateAlt(double timestep)
{
   double vscmd = kalt * (player->autoPilotCmds.getCmdAltitude() - player->alt);

   vscmd =
       std::min(std::fabs(vscmd), player->autoPilotCmds.getMaxVS()) * UnitConvert::signum(vscmd);

   updateVS(timestep, vscmd);
}

void AutoPilot::updateVS(double timestep, double cmdVs)
{
   double u = player->uvw.get1();
   double phi = player->eulers.getPhi();
   double theta = player->eulers.getTheta();
   double q = player->pqr.get2();
   double thetaDenom =
       player->autoPilotCmds.getMaxPitchUp() - player->autoPilotCmds.getMaxPitchDown();

   // setup the commanded pitch rate based on difference in vertical speed.  Based on the
   // approximation that
   // deltaVS = vInf * dTheta.  for straight and level: deltaVS = uq, so q = deltaVS / u
   // cmdVs is a positive up quantity, nedVel[3] is a negative up quantity
   double qCmd =
       (cmdVs + player->nedVel.get3() + kpitch * (player->nedVel.get3() - lastVz) / timestep) /
       u;
   lastVz = player->nedVel.get3();

   // amount of q to compensate for turning ( extra back pressure to compensate for r pulling
   // downward)
   double qTurn = player->pqr.get3() * std::tan(phi);

   if (qCmd > 0) {
      double qLimit = std::min((player->autoPilotCmds.getMaxPitchUp() - theta) /
                                   player->autoPilotCmds.getMaxPitchUp(),
                               1.0);
      double qMax = maxG / u * qLimit;
      qCmd = std::min(qCmd, qMax);
      qCmd = std::min(q + maxG_rate / u * timestep, qCmd);
   } else {
      double qLimit = std::min((player->autoPilotCmds.getMaxPitchDown() - theta) /
                                   player->autoPilotCmds.getMaxPitchDown(),
                               1.0);
      double qMin = minG / u * qLimit;
      qCmd = std::max(qMin, qCmd);
      qCmd = std::max(q - qTurn + minG_rate / u * timestep, qCmd);
   }

   // if in a turn, add in extra q to compensate for r pulling downward
   qCmd += qTurn;

   player->pqr.set2(qCmd);

   //        //double cmdVs = player->autoPilotCmds.getCmdVertSpeed();
   //
   //        //if turning and commanding descent, complete turn , then descend
   //        if ( std::fabs(phi) > 0.1 && cmdVs < 0) {
   //            cmdVs = 0.;
   //        }
   //
   //        double dq = (qCmd - player->pqr.get2()) / timestep;
   //        double dqmax = qMax * timestep;
   //        dq = min(dq, dqmax);
   //
   //        // check for max pitch angles
   //        //double pitch = player->eulers.getTheta();
   //        //if (pitch > player->autoPilotCmds.getMaxPitchUp() && qCmd > 0) qCmd = 0;
   //        //if ( pitch < player->autoPilotCmds.getMaxPitchDown() && qCmd < 0) qCmd = 0;
   //
   //        // don't allow more than gMax accel from q
   //        qCmd = min( qMax, max( qMin, qCmd ) );
   //
   //        player->pqr.set2( player->pqr.get2() + dq );
}

void AutoPilot::updateSpeed(double timestep)
{
   double cmdVel = player->autoPilotCmds.getCmdSpeed();
   if (player->autoPilotCmds.isUsingMach()) {
      cmdVel = player->autoPilotCmds.getCmdMach() *
               Atmosphere::getSpeedSound(Atmosphere::getTemp(player->alt));
   }

   const double dV = (cmdVel - player->uvw.get1()) - player->uvwdot.get1() * spoolTime;
   const double dT = timestep / spoolTime * dV;

   // double dV = ( cmdVel - player->uvw.get1() );
   // double dT = dV * (1- exp(-timestep/spoolTime));

   player->throttle = player->throttle + dT;

   if (player->throttle < minThrottle) {
      player->throttle = minThrottle;
   } else if (player->throttle > maxThrottle) {
      player->throttle = maxThrottle;
   }
}
}
}
