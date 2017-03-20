
#ifndef __AutoPilotCmds_H__
#define __AutoPilotCmds_H__

namespace sf {

//------------------------------------------------------------------------------
// Class: AutoPilotCmds
//------------------------------------------------------------------------------
class AutoPilotCmds
{
public:
   AutoPilotCmds();
   ~AutoPilotCmds();

   void setAutoPilotOn(bool onOff);
   bool isAutoPilotOn();

   void setAutoThrottleOn(bool onOff);
   bool isAutoThrottleOn();

   void setCmdHeading(double radHeading);
   double getCmdHeading();

   void setCmdAltitude(double metersMSLAlt);
   double getCmdAltitude();

   void setCmdVertSpeed(double metersPerSec);
   double getCmdVertSpeed();

   void setCmdSpeed( double metersPerSec );
   double getCmdSpeed();

   bool isAltHoldOn();
   void setAltHoldOn(bool altHoldOn);

   bool isVsHoldOn();
   void setVsHoldOn(bool vsHoldOn);

   bool isHdgHoldOn();
   void setHdgHoldOn(bool hdgHoldOn);

   bool isOrbitHoldOn();
   void setOrbitHoldOn(bool orbitHoldOn);

   bool isLevelOn();
   void setLevelOn(bool levelOn);

   void setMaxPitchUp(double radPitch);
   double getMaxPitchUp();

   void setMaxPitchDown(double radPitch);
   double getMaxPitchDown();

   void setMaxBank(double radbank);
   double getMaxBank();

   void setMaxVS(double vs);
   double getMaxVS();

   void setUseMach(bool useMach);
   bool isUsingMach();

   void setCmdMach(double mach);
   double getCmdMach();

   void setCmdSideSlip(double mpsSideslip);
   double getCmdSideSlip();

protected:

   double vel {};
   double alt {};
   double vs {};
   double hdg {};
   double mach {};
   double sideslip {};

   bool apOn {};
   bool atOn {};
   bool altHoldOn {};
   bool vsHoldOn {};
   bool hdgHoldOn {};
   bool orbitHoldOn {};
   bool levelOn {};
   bool useMach {};

   double maxPitch {};
   double minPitch {};
   double maxBank {};
   double maxVS {};
};

}

#endif
