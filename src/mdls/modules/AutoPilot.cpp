
#include "sflight/mdls/modules/AutoPilot.hpp"

#include "sflight/mdls/modules/Atmosphere.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/node_utils.hpp"

#include "sflight/mdls/Player.hpp"
#include "sflight/mdls/UnitConvert.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace sflight {
namespace mdls {

AutoPilot::AutoPilot(Player* globals, const double frameRate) : Module(globals, frameRate)
{
}

AutoPilot::~AutoPilot() {}

void AutoPilot::initialize(xml::Node* node)
{
   xml::Node* apProps = node->getChild("AutoPilot");

   std::vector<xml::Node*> comps = xml::getList(apProps, "Component");

   for (int i = 0; i < comps.size(); i++) {
      xml::Node* tmp = comps[i];

      if (xml::get(tmp, "Type", "") == "HeadingHold") {
         globals->autoPilotCmds.setMaxBank(
             UnitConvert::toRads(xml::getDouble(tmp, "MaxBank", 30)));
         kphi = xml::getDouble(tmp, "BankWeight", kphi);
         maxBankRate = UnitConvert::toRads(xml::getDouble(tmp, "MaxBankRate", maxBankRate));
         turnType = xml::get(tmp, "TurnType", "").find("TRAJECTORY") == 0
                        ? TurnType::TRAJECTORY
                        : TurnType::HDG;
      } else if (xml::get(tmp, "Type", "") == "AltitudeHold") {
         globals->autoPilotCmds.setMaxVS(
             UnitConvert::FPMtoMPS(xml::getDouble(tmp, "MaxVS", 0)));
         kalt = xml::getDouble(tmp, "AltWeight", 0.2);
      } else if (xml::get(tmp, "Type", "") == "VSHold") {
         maxG = (xml::getDouble(tmp, "MaxG", maxG) - 1) * nav::getG(0, 0, 0);
         minG = (xml::getDouble(tmp, "MinG", minG) - 1) * nav::getG(0, 0, 0);
         maxG_rate = maxG;
         minG_rate = minG;
         globals->autoPilotCmds.setMaxPitchUp(
             UnitConvert::toRads(xml::getDouble(tmp, "MaxPitchUp", math::PI / 2.0)));
         globals->autoPilotCmds.setMaxPitchDown(
             UnitConvert::toRads(xml::getDouble(tmp, "MaxPitchDown", -math::PI / 2.0)));
         kpitch = xml::getDouble(tmp, "PitchWeight", 0);
      } else if (xml::get(tmp, "Type", "") == "AutoThrottle") {
         maxThrottle = xml::getDouble(tmp, "MaxThrottle", maxThrottle);
         minThrottle = xml::getDouble(tmp, "MinThrottle", minThrottle);
         spoolTime = xml::getDouble(tmp, "SpoolTime", spoolTime);
      }
   }

   hdgErrTol = UnitConvert::toRads(2);
   globals->autoPilotCmds.setAutoPilotOn(true);
   globals->autoPilotCmds.setAutoThrottleOn(true);
}

void AutoPilot::update(const double timestep)
{
   if (globals->autoPilotCmds.isAutoPilotOn()) {

      // null rotational accel rates ( in case of previous manual control )
      globals->pqrdot.multiply(0);

      if (globals->autoPilotCmds.isOrbitHoldOn()) {
         updateHdg(timestep, globals->eulers.getPsi() +
                                 math::PI / 2.0 *
                                     UnitConvert::signum(globals->autoPilotCmds.getMaxBank()));
         updateAlt(timestep);
      }

      if (globals->autoPilotCmds.isAltHoldOn()) {
         updateAlt(timestep);
      } else if (globals->autoPilotCmds.isVsHoldOn()) {
         updateVS(timestep, globals->autoPilotCmds.getCmdVertSpeed());
      }

      if (globals->autoPilotCmds.isHdgHoldOn()) {
         updateHdg(timestep, globals->autoPilotCmds.getCmdHeading());
      }
   }

   if (globals->autoPilotCmds.isAutoThrottleOn()) {
      updateSpeed(timestep);
   }
}

void AutoPilot::updateHdg(double timestep, double cmdHdg)
{
   double hdgDiff{};

   if (turnType == TurnType::TRAJECTORY) {
      hdgDiff = cmdHdg - std::atan2(globals->nedVel.get2(), globals->nedVel.get1());
   } else {
      hdgDiff = cmdHdg - globals->eulers.getPsi();
   }

   // if hdgDiff > 180, turn opp dir by making hdgDiff neg num <180
   hdgDiff = UnitConvert::wrapHeading(hdgDiff, true);

   double phiCmd = kphi * hdgDiff * globals->vInf * maxBankRate;
   // bound phiCmd to -maxBank...maxBank
   phiCmd = std::min(globals->autoPilotCmds.getMaxBank(),
                     std::max(-globals->autoPilotCmds.getMaxBank(), phiCmd));

   // bound pCmd to -maxBankRate...maxBankRate
   const double pCmd =
       std::min(maxBankRate, std::max(-maxBankRate, phiCmd - globals->eulers.getPhi()));

   globals->pqr.set1(pCmd);
}

void AutoPilot::updateAlt(double timestep)
{
   double vscmd = kalt * (globals->autoPilotCmds.getCmdAltitude() - globals->alt);

   vscmd =
       std::min(fabs(vscmd), globals->autoPilotCmds.getMaxVS()) * UnitConvert::signum(vscmd);

   updateVS(timestep, vscmd);
}

void AutoPilot::updateVS(double timestep, double cmdVs)
{
   double u = globals->uvw.get1();
   double phi = globals->eulers.getPhi();
   double theta = globals->eulers.getTheta();
   double q = globals->pqr.get2();
   double thetaDenom =
       globals->autoPilotCmds.getMaxPitchUp() - globals->autoPilotCmds.getMaxPitchDown();

   // setup the commanded pitch rate based on difference in vertical speed.  Based on the
   // approximation that
   // deltaVS = vInf * dTheta.  for straight and level: deltaVS = uq, so q = deltaVS / u
   // cmdVs is a positive up quantity, nedVel[3] is a negative up quantity
   double qCmd = (cmdVs + globals->nedVel.get3() +
                  kpitch * (globals->nedVel.get3() - lastVz) / timestep) /
                 u;
   lastVz = globals->nedVel.get3();

   // amount of q to compensate for turning ( extra back pressure to compensate for r pulling
   // downward)
   double qTurn = globals->pqr.get3() * tan(phi);

   if (qCmd > 0) {
      double qLimit = std::min((globals->autoPilotCmds.getMaxPitchUp() - theta) /
                                   globals->autoPilotCmds.getMaxPitchUp(),
                               1.0);
      double qMax = maxG / u * qLimit;
      qCmd = std::min(qCmd, qMax);
      qCmd = std::min(q + maxG_rate / u * timestep, qCmd);
   } else {
      double qLimit = std::min((globals->autoPilotCmds.getMaxPitchDown() - theta) /
                                   globals->autoPilotCmds.getMaxPitchDown(),
                               1.0);
      double qMin = minG / u * qLimit;
      qCmd = std::max(qMin, qCmd);
      qCmd = std::max(q - qTurn + minG_rate / u * timestep, qCmd);
   }

   // if in a turn, add in extra q to compensate for r pulling downward
   qCmd += qTurn;

   globals->pqr.set2(qCmd);

   //        //double cmdVs = globals->autoPilotCmds.getCmdVertSpeed();
   //
   //        //if turning and commanding descent, complete turn , then descend
   //        if ( fabs(phi) > 0.1 && cmdVs < 0) {
   //            cmdVs = 0.;
   //        }
   //
   //        double dq = (qCmd - globals->pqr.get2()) / timestep;
   //        double dqmax = qMax * timestep;
   //        dq = min(dq, dqmax);
   //
   //        // check for max pitch angles
   //        //double pitch = globals->eulers.getTheta();
   //        //if (pitch > globals->autoPilotCmds.getMaxPitchUp() && qCmd > 0) qCmd = 0;
   //        //if ( pitch < globals->autoPilotCmds.getMaxPitchDown() && qCmd < 0) qCmd = 0;
   //
   //        // don't allow more than gMax accel from q
   //        qCmd = min( qMax, max( qMin, qCmd ) );
   //
   //        globals->pqr.set2( globals->pqr.get2() + dq );
}

void AutoPilot::updateSpeed(double timestep)
{
   double cmdVel = globals->autoPilotCmds.getCmdSpeed();
   if (globals->autoPilotCmds.isUsingMach()) {
      cmdVel = globals->autoPilotCmds.getCmdMach() *
               Atmosphere::getSpeedSound(Atmosphere::getTemp(globals->alt));
   }

   double dV = (cmdVel - globals->uvw.get1()) - globals->uvwdot.get1() * spoolTime;
   double dT = timestep / spoolTime * dV;

   // double dV = ( cmdVel - globals->uvw.get1() );
   // double dT = dV * (1- exp(-timestep/spoolTime));

   globals->throttle = globals->throttle + dT;

   if (globals->throttle < minThrottle) {
      globals->throttle = minThrottle;
   } else if (globals->throttle > maxThrottle) {
      globals->throttle = maxThrottle;
   }
}
}
}
