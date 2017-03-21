
#ifndef __PID_H__
#define __PID_H__

namespace sf
{

//------------------------------------------------------------------------------
// Class: PID
//------------------------------------------------------------------------------
class PID
{
 public:
   PID() = default;
   PID(const double minVal, const double maxVal, const double kp);
   virtual ~PID() = default;

   void setIDTimes(const double ti, const double td);

   void setGain(const double p);

   double getP() const;
   double getI() const;
   double getD() const;

   double getOutput(const double timestep, const double desired_pt,
                    const double current_pt, const double current_output);

   double kp {};
   double alpha {0.1}, beta {1.0}, gamma {};
   double ts {}, ti {}, td {}, tf {};
   double en {};
   double rn {}, yn {};
   double max {}, min {};
   double du {}, u {};

   double ep[2] {};
   double ed[3] {};
   double edf[4] {};
};
}

#endif