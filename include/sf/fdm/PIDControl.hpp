
#ifndef __PIDConttol_H__
#define __PIDConttol_H__

#include "sf/fdm/PIControl.hpp"

namespace sf
{

//------------------------------------------------------------------------------
// Class: PIDControl
//------------------------------------------------------------------------------
class PIDControl : public PIControl
{

 public:
   PIDControl();
   PIDControl(const double minVal, const double maxVal, const double p,
              const double i, const double d);
   ~PIDControl();

   virtual double getD() const;
   virtual void setD(const double d);

   virtual double getOutput(const double timestep, const double desired_pt,
                            const double current_pt, const double current_output);

 protected:
   double edf0 {}, edf1 {}, edf2 {};
   double tstf {};
   double ed {};
};

}

#endif
