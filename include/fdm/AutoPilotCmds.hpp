
#ifndef __AutoPilotCmds_H__
#define __AutoPilotCmds_H__

namespace sf {

//------------------------------------------------------------------------------
// Class: AutoPilotCmds
//------------------------------------------------------------------------------
class AutoPilotCmds
{
 public:
   AutoPilotCmds() = default;
   virtual ~AutoPilotCmds() = default;

   void setAutoPilotOn(const bool x)                 { apOn = x;           }
   bool isAutoPilotOn() const                        { return apOn;        }

   void setAutoThrottleOn(const bool x)              { atOn = x;           }
   bool isAutoThrottleOn() const                     { return atOn;        }

   // commanded heading (radians)
   void setCmdHeading(const double radHeading);
   double getCmdHeading() const                      { return hdg;         }

   // MSL altitude (meters)
   void setCmdAltitude(const double x)               { alt = x;            }
   double getCmdAltitude() const                     { return alt;         }

   // commanded vertical speed (m/s)
   void setCmdVertSpeed(const double x)              { vs = x;             }
   double getCmdVertSpeed() const                    { return vs;          }

   // commanded speed (m/s)
   void setCmdSpeed(const double x)                  { vel = x;            }
   double getCmdSpeed() const                        { return vel;         }

   void setCmdMach(const double x)                   { mach = x;           }
   double getCmdMach() const                         { return mach;        }

   void setUseMach(const bool x)                     { useMach = x;        }
   bool isUsingMach() const                          { return useMach;     }

   // side slip (m/s)
   void setCmdSideSlip(const double x)               { sideslip = x;       }
   double getCmdSideSlip() const                     { return sideslip;    }

   void setAltHoldOn(const bool x)                   { altHoldOn = x;      }
   bool isAltHoldOn() const                          { return altHoldOn;   }

   void setVsHoldOn(const bool x)                    { vsHoldOn = x;       }
   bool isVsHoldOn() const                           { return vsHoldOn;    }

   void setHdgHoldOn(const bool x)                   { hdgHoldOn = x;      }
   bool isHdgHoldOn() const                          { return hdgHoldOn;   }

   void setOrbitHoldOn(const bool x)                 { orbitHoldOn = x;    }
   bool isOrbitHoldOn() const                        { return orbitHoldOn; }

   void setLevelOn(const bool x)                     { levelOn = x;        }
   bool isLevelOn() const                            { return levelOn;     }

   // max pitch up (in )radians)
   void setMaxPitchUp(const double x)                { maxPitch = x;       }
   double getMaxPitchUp() const                      { return maxPitch;    }

   // max pitch down (radians)
   void setMaxPitchDown(const double x)              { minPitch = x;       }
   double getMaxPitchDown() const                    { return minPitch;    }

   // max bank (radians)
   void setMaxBank(const double x)                   { maxBank = x;        }
   double getMaxBank() const                         { return maxBank;     }

   void setMaxVS(const double x)                     { maxVS = x;          }
   double getMaxVS() const                           { return maxVS;       }

 private:
   double vel {};
   double alt {};        // meters
   double vs {};
   double hdg {};        // radians
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

   double maxPitch {};   // radians
   double minPitch {};
   double maxBank {};    // radians
   double maxVS {};
};
}

#endif
